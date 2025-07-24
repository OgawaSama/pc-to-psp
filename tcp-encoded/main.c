/*
 * PSP Software Development Kit - https://github.com/pspdev
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in PSPSDK root for details.
 *
 * main.c - Simple elf based network example.
 *
 * Copyright (c) 2005 James F <tyranid@gmail.com>
 * Some small parts (c) 2005 PSPPet
 *
 */


#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdint.h>

#include <arpa/inet.h>

#include <pspkernel.h>
#include <pspnet_apctl.h>
#include <pspsdk.h>
#include <psputility.h>
#include <psptypes.h>
#include <pspctrl.h>


#define printf pspDebugScreenPrintf
#define MAX_BUFFER 2048

#define MODULE_NAME "TCP"
#define HELLO_MSG "Hello from the PSP! TCP Connection established!\r\n"

PSP_MODULE_INFO(MODULE_NAME, 0, 1, 1);
PSP_HEAP_THRESHOLD_SIZE_KB(1024);
PSP_HEAP_SIZE_KB(-2048);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);
PSP_MAIN_THREAD_STACK_SIZE_KB(1024);

/* Exit callback */
int exit_callback(int arg1, int arg2, void *common) {
	sceKernelExitGame();
	return 0;
}

/* Callback thread */
int CallbackThread(SceSize args, void *argp) {
	int cbid;

	cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();

	return 0;
}

/* Sets up the callback thread and returns its thread id */
int SetupCallbacks(void) {
	int thid = 0;

	thid =
			sceKernelCreateThread("update_thread", CallbackThread, 0x11, 0xFA0, PSP_THREAD_ATTR_USER, 0);
	if (thid >= 0)
	{
		sceKernelStartThread(thid, 0, 0);
	}

	return thid;
}

#define SERVER_PORT 23

int button_monitor(SceSize args, void* argp) {
	int pipe_fd = *(int*) argp;
	SceCtrlData pad;
	u32 prev_buttons = 0;
	while (1) {
		sceCtrlPeekBufferPositive(&pad, 1);
		
		// notify for button changes
		// maybe remove?
		if (pad.Buttons != prev_buttons) {
			char pipecleaner = 1;
			write(pipe_fd, &pipecleaner, 1);
			prev_buttons = pad.Buttons;
		}

		sceKernelDelayThread(10000);
	}

	return 0;
}

int check_button(char* msg, SceCtrlData *pad) {
	int flag = 0; 	// for start and select flags
	uint32_t buttonsPressed = 0x00000000;	// booleans for which buttons were pressed

	// analog sticks
	uint32_t Lx = ((uint32_t) pad->Lx) << 24;
	uint32_t Ly = ((uint32_t) pad->Ly) << 16;
	buttonsPressed = buttonsPressed | Lx;
	buttonsPressed = buttonsPressed | Ly;

	// directions
	if (pad->Buttons & PSP_CTRL_LEFT)
	{	
		buttonsPressed = buttonsPressed | 0x00000080;
	}
	if (pad->Buttons & PSP_CTRL_UP)
	{
		buttonsPressed = buttonsPressed | 0x00000040;
	}
	if (pad->Buttons & PSP_CTRL_RIGHT)
	{
		buttonsPressed = buttonsPressed | 0x00000020;
	}
	if (pad->Buttons & PSP_CTRL_DOWN)
	{
		buttonsPressed = buttonsPressed | 0x00000010;
	}

	// symbols
	if (pad->Buttons & PSP_CTRL_SQUARE)
	{
		buttonsPressed = buttonsPressed | 0x00000008;
	}
	if (pad->Buttons & PSP_CTRL_TRIANGLE)
	{
		buttonsPressed = buttonsPressed | 0x00000004;
	}
	if (pad->Buttons & PSP_CTRL_CIRCLE)
	{
		buttonsPressed = buttonsPressed | 0x00000002;
	}
	if (pad->Buttons & PSP_CTRL_CROSS)
	{
		buttonsPressed = buttonsPressed | 0x00000001;
	}

	// bumpers
	if (pad->Buttons & PSP_CTRL_LTRIGGER)
	{
		buttonsPressed = buttonsPressed | 0x00000200;
	}
	if (pad->Buttons & PSP_CTRL_RTRIGGER)
	{
		buttonsPressed = buttonsPressed | 0x00000100;
	}

	// extras
	if (pad->Buttons & PSP_CTRL_START)
	{
		buttonsPressed = buttonsPressed | 0x00000400;
		flag = 1;
	}
	if (pad->Buttons & PSP_CTRL_SELECT)
	{
		buttonsPressed = buttonsPressed | 0x00000800;
		flag = 2;
	}

	// kernel mode only :(
	if (pad->Buttons & PSP_CTRL_SCREEN)
	{
		buttonsPressed = buttonsPressed | 0x00008000;
	}
	if (pad->Buttons & PSP_CTRL_HOLD)
	{
		buttonsPressed = buttonsPressed | 0x00004000;
	}
	if (pad->Buttons & PSP_CTRL_NOTE)
	{
		buttonsPressed = buttonsPressed | 0x00002000;
	}
	if (pad->Buttons & PSP_CTRL_HOME)
	{
		buttonsPressed = buttonsPressed | 0x00001000;
	}

	// conver the 32bit uint into data for TCP functions
	uint32_t convert = htonl(buttonsPressed);
	memcpy(msg, (char*) &convert, sizeof(uint32_t));

	printf("Sending %lx\n", buttonsPressed);

	return flag;
}

int make_socket(uint16_t port) {
	int sock;
	int ret;
	struct sockaddr_in name;

	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
	{
		return -1;
	}

	name.sin_family = AF_INET;
	name.sin_port = htons(port);
	name.sin_addr.s_addr = htonl(INADDR_ANY);
	ret = bind(sock, (struct sockaddr *)&name, sizeof(name));
	if (ret < 0)
	{
		return -1;
	}

	return sock;
}

/* Start a simple tcp echo server */
void start_server(const char *szIpAddr) {
    int server_fd;
    int client_fd = -1;
	struct sockaddr_in client;
    socklen_t client_len = sizeof(client);
	char data[1024];
	fd_set readfds;

	// To check for button presses
    SceCtrlData pad;
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
	int button_pipe[2];
	if (pipe(button_pipe) < 0) {
		printf("Error creating button pipe\n");
		return;
	}
	int button_thread = sceKernelCreateThread("button_thread", button_monitor,
												0x18, 
												0x1000, 0, NULL);
	if (button_thread < 0) {
		printf("Error creating button thread\n");
		close(button_pipe[0]);
		close(button_pipe[1]);
		return;
	}
	sceKernelStartThread(button_thread, sizeof(int), &button_pipe[1]);


	if ((server_fd = make_socket(SERVER_PORT)) < 0) {
		printf("Error creating server socket\n");
		close(button_pipe[0]);
		close(button_pipe[1]);
		sceKernelDeleteThread(button_thread);
		return;
	}

	if ((listen(server_fd, 1)) < 0) {
		printf("Error calling listen\n");
        close(server_fd);
		close(button_pipe[0]);
		close(button_pipe[1]);
		sceKernelDeleteThread(button_thread);
		return;
	}

	printf("Listening for connections ip %s port %d\n", szIpAddr, SERVER_PORT);

    
	while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
		FD_SET(button_pipe[0], &readfds);
        if (client_fd != -1) {
            FD_SET(client_fd, &readfds);
        }

 
		if (select(FD_SETSIZE, &readfds, NULL, NULL, NULL) < 0) {
			printf("select error\n");
			return;
		}

        if (FD_ISSET(server_fd, &readfds)) {
            client_fd = accept(server_fd, (struct sockaddr*) &client, &client_len);
            if (client_fd < 0) {
                printf("Error in accept %s\n", strerror(errno));
                close(server_fd);
                break;
            }

            printf("New connection from %s:%d\n", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
            write(client_fd, HELLO_MSG, strlen(HELLO_MSG));


        }

		// always open this cause there IS a client
		if (FD_ISSET(client_fd, &readfds)) {
            int readbytes = read(client_fd, data, MAX_BUFFER - 1);
            if (readbytes <= 0 ) {
                printf("Client disconnected\n");
                close(client_fd);
                client_fd = -1;
            } 

			data[readbytes] = '\0';
			// printf("Received: %.*s", readbytes, data);
			printf("Received: %s", data);
			//write(client_fd, data, readbytes);

			if (strcmp(data, "exit") == 0) {
                printf("Peer has exited, closing connection.\n");
				close(client_fd);
                break;
            }
            
        }

		if (FD_ISSET(button_pipe[0], &readfds)) {
			char pipecleaner;
			read(button_pipe[0], &pipecleaner, 1);

			sceCtrlPeekBufferPositive(&pad, 1);
			if (pad.Buttons != 0) {
				int ret = check_button(data, &pad);
				if (ret < 0) {
					printf("Error in button press %s\n", strerror(errno));
					close(client_fd);
					break;
				}
				if (ret == 1) {
					printf("Start pressed. Kicking client\n");
					if (client_fd != -1) {
						send(client_fd, "exit", strlen("exit"), 0);
					}
				}
				if (ret == 2) {
					printf("Select pressed. Clearing screen\n");
					pspDebugScreenClear();
				}
				
				if (client_fd != -1 && ret != 1) {	// avoid sending additional msgs on kicks
					send(client_fd, data, strlen(data), 0);
					//printf("Sent: %s\n", data);
				}
	
				sceKernelDelayThread(2000);
			}
		}
	}

	if (client_fd != -1)
        close(client_fd);
    close(server_fd);
	close(button_pipe[0]);
	close(button_pipe[1]);
	sceKernelDeleteThread(button_thread);
}

/* Connect to an access point */
int connect_to_apctl(int config) {
	int err;
	int stateLast = -1;

	/* Connect using the first profile */
	err = sceNetApctlConnect(config);
	if (err != 0)
	{
		printf(MODULE_NAME ": sceNetApctlConnect returns %08X\n", err);
		return 0;
	}

	printf(MODULE_NAME ": Connecting...\n");
	while (1)
	{
		int state;
		err = sceNetApctlGetState(&state);
		if (err != 0)
		{
			printf(MODULE_NAME ": sceNetApctlGetState returns $%x\n", err);
			break;
		}
		if (state > stateLast)
		{
			printf("  connection state %d of 4\n", state);
			stateLast = state;
		}
		if (state == 4)
			break; // connected with static IP

		// wait a little before polling again
		sceKernelDelayThread(50 * 1000); // 50ms
	}
	printf(MODULE_NAME ": Connected!\n");

	if (err != 0)
	{
		return 0;
	}

	return 1;
}

int net_thread(SceSize args, void *argp) {
	int err;
	do
	{
		if ((err = pspSdkInetInit()))
		{
			printf(MODULE_NAME ": Error, could not initialise the network %08X\n", err);
			break;
		}

		// if you're having issues with connecting to the internet, check here:
		if (connect_to_apctl(1))	// connects to the first connection listed on the PSP
		{
			// connected, get my IPADDR and run test
			union SceNetApctlInfo info;

			if (sceNetApctlGetInfo(8, &info) != 0)
				strcpy(info.ip, "unknown IP");
			start_server(info.ip);
		}
	} while (0);

	return 0;
}

/* Simple thread */
int main(int argc, char **argv) {
	SceUID thid;

	SetupCallbacks();

	sceUtilityLoadNetModule(PSP_NET_MODULE_COMMON);
	sceUtilityLoadNetModule(PSP_NET_MODULE_INET);

	pspDebugScreenInit();

	/* Create a user thread to do the real work */
	thid = sceKernelCreateThread("net_thread", net_thread,
															 0x11,			 // default priority
															 256 * 1024, // stack size (256KB is regular default)
															 PSP_THREAD_ATTR_USER, NULL);
	if (thid < 0)
	{
		printf("Error, could not create thread\n");
		sceKernelSleepThread();
	}

	sceKernelStartThread(thid, 0, NULL);

	sceKernelExitDeleteThread(0);

	return 0;
}