## Streaming PC to PSP

This started as a project to stream my PC into my PSP via internet.  
As porting Moonlight in its entirety would be too tiresome and honestly too much for me, I settled down to just doing the necessary.  
My view for this project was to make some way to:
* Send PSP inputs to the PC to use as a controller (even using the analog as a way to control the mouse cursor);
* Send PC audio to the PSP (even if low quality);
* Send PC video to the PSP (even if low quality);

But as I found out, **it is very hard to do so**. So I gave up for the moment.  
I plan to later come back here and finish what I started, but for now, as someone who never programmed for the PSP, this is my limit.  


# What can you find here though?
Just a way to send PSP input into the PC and vice-versa. The PC input must be sent via text on the terminal.  
This is based on a sample from PSPDEV's github found [here](https://github.com/pspdev/pspsdk/blob/master/src/samples/net/simple/main.c).  
Not much has been changed, just now you can send PSP input by pressing the buttons.  
This repo includes two versions:
* Plaintext-tcp: The messages sent are just plaintext messages such as "Left trigger pressed!", nothing too fancy here.
* Encoded-tcp: This was my attempt at sending multiple button presses at once. When preparing the PSP message, it creates a 32bit integer with bool flags corresponding to the pressed buttons, as well as the analog stick (x,y) values.  

As though this worked, the PC and PSP terminals lags when you spam, so be careful with that.
Alongside these basic programs, I tried programming a simple GUI to showcase what buttons were being pressed, but I couldn't really make this work properly, so I always seem to get stack overflows :/  

# How to try for yourself
Choose a version that you want to try and compile the `client.c` with gcc. No flags needed.
To compile the `main.c` and generate the EBOOT.PBP needed, just use the Makefile with `make`.
After that, transfer the EBOOT.PBP to your PSP's memory card (tested only with CFW) and run the program.
On PC, run the generated client object with the desired IP and port you want to access (these will be shown on the PSP's screen). 

If you want to try with the GUI (not that fun, to be honest), simply compile the `gui.c` file with
```shell
gcc $( pkg-config --cflags gtk4 ) -o gui.o gui.c $( pkg-config --libs gtk4 )
```
and run the GUI after piping from the client. Example:
```shell
./client.o {ip} 52 | ./gui
```

If you wish to chat with another person instead of you PSP, you can compile and run `server.c` and tell your friend your IP and chosen port.  


-------
## That's all for now
Thank you very much if you're reading this!
As I said, this right now was my limit. But, if you wish to help me out on this project or just want to ask a question, feel free to contact me at anytime, be it via github or somewhere else!  
