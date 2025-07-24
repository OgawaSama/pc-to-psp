#include <gtk/gtk.h>
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <arpa/inet.h>

/* 
Compile GUI with
gcc $( pkg-config --cflags gtk4 ) -o gui.o gui.c $( pkg-config --libs gtk4 )
*/

#define MAX_BUFFER 16384

typedef struct {
    // This is so goddamn ugly, I need to fix this sometime
    GtkWidget* statusBar;
    GtkWidget* lt;
    GtkWidget* rt;
    GtkWidget* left;
    GtkWidget* right;
    GtkWidget* up;
    GtkWidget* down;
    GtkWidget* square;
    GtkWidget* triangle;
    GtkWidget* circle;
    GtkWidget* cross;
    GtkWidget* lx;
    GtkWidget* ly;
    GtkWidget* lxValue;
    GtkWidget* lyValue;
    GtkWidget* home;
    GtkWidget* hold;
    GtkWidget* screen;
    GtkWidget* note;
    GtkWidget* select;
    GtkWidget* start;
} AppWidgets;

AppWidgets* widgetList;

static gboolean updateStatusBar (gpointer user_data) {
    char* text = (char*) user_data;
    gtk_label_set_text(GTK_LABEL(widgetList->statusBar), text);
    g_free(text);
    return G_SOURCE_REMOVE;
}

static void setTextColor(GtkWidget* text, char* color) {
    char pangoString[256] = "<span foreground=\"";
    strcat(pangoString, color);
    strcat(pangoString, "\">");
    strcat(pangoString, gtk_label_get_label(GTK_LABEL(text)));
    strcat(pangoString, "</span>");

    gtk_label_set_markup(GTK_LABEL(text), pangoString);
}

static gboolean updateButtons (gpointer user_data) {
    char* received_input = (char*) user_data;
    uint32_t input;
    memcpy(&input, received_input, sizeof(uint32_t));
    uint32_t inputFormatted = ntohl(input);

    // analog   
        // this analog part may be broken
    unsigned char Lx[1];
    Lx[0] = (inputFormatted >> 24);
    gtk_label_set_text(GTK_LABEL(widgetList->lxValue), Lx);
    unsigned char Ly[1];
    Ly[0 ]= ((inputFormatted & 0x00880000) >> 16);
    gtk_label_set_text(GTK_LABEL(widgetList->lyValue), Ly);
    // directions
    if ((inputFormatted & 0x00000080) >> 4) {
        setTextColor(widgetList->left, "red");
    } else setTextColor(widgetList->left, "white");
    if ((inputFormatted & 0x00000040) >> 4) {
        setTextColor(widgetList->up, "red");
    } else setTextColor(widgetList->up, "white");
    if ((inputFormatted & 0x00000020) >> 4) {
        setTextColor(widgetList->right, "red");
    } else setTextColor(widgetList->right, "white");
    if ((inputFormatted & 0x00000010) >> 4) {
        setTextColor(widgetList->down, "red");
    } else setTextColor(widgetList->down, "white");
    // symbols
    if ((inputFormatted & 0x00000008)) {
        setTextColor(widgetList->square, "red");
    } else setTextColor(widgetList->square, "white");
    if ((inputFormatted & 0x00000004)) {
        setTextColor(widgetList->triangle, "red");
    } else setTextColor(widgetList->triangle, "white");
    if ((inputFormatted & 0x00000002)) {
        setTextColor(widgetList->circle, "red");
    } else setTextColor(widgetList->circle, "white");
    if ((inputFormatted & 0x00000001)) {
        setTextColor(widgetList->cross, "red");
    } else setTextColor(widgetList->cross, "white");
    // extra
    if ((inputFormatted & 0x00000800) >> 8) {
        setTextColor(widgetList->select, "red");
    } else setTextColor(widgetList->select, "white");
    if ((inputFormatted & 0x00000400) >> 8) {
        setTextColor(widgetList->start, "red");
    } else setTextColor(widgetList->start, "white");
    if ((inputFormatted & 0x00000200) >> 8) {
        setTextColor(widgetList->lt, "red");
    } else setTextColor(widgetList->lt, "white");
    if ((inputFormatted & 0x00000100) >> 8) {
        setTextColor(widgetList->rt, "red");
    } else setTextColor(widgetList->rt, "white");
    // kernel only
    if ((inputFormatted & 0x00008000) >> 12) {
        setTextColor(widgetList->screen, "red");
    } else setTextColor(widgetList->screen, "white");
    if ((inputFormatted & 0x00004000) >> 12) {
        setTextColor(widgetList->hold, "red");
    } else setTextColor(widgetList->hold, "white");
    if ((inputFormatted & 0x00002000) >> 12) {
        setTextColor(widgetList->note, "red");
    } else setTextColor(widgetList->note, "white");
    if ((inputFormatted & 0x00001000) >> 12) {
        setTextColor(widgetList->home, "red");
    } else setTextColor(widgetList->home, "white");

}


static void* stdin_thread(void* data) {
    fd_set readfds;
    char buffer[4] = { 0 };
    while(1) {
        //maybe change all of this loop? or not idk
        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int activity = select(FD_SETSIZE, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(buffer, sizeof(buffer), stdin) == NULL)  {
                perror("fgets error");
                break;
            }

            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') {
                buffer[len-1] = '\0';   
            }

            if (buffer[0] == 'C') {
                // update connection status
                char* label_text = g_strdup(buffer);
                memmove(label_text, label_text+2, strlen(label_text));
                g_idle_add(updateStatusBar, label_text);
            }

            if (buffer[0] == 'I') {
                // update buttons
                char* received_input = g_strdup(buffer);
                memmove(received_input, received_input+2, strlen(received_input));
                g_idle_add(updateButtons, received_input);
            }

        }
    }

    return NULL;
}

static void activate (GtkApplication* app, gpointer user_data) {
    GtkWidget* button;

    GtkWidget* window = gtk_application_window_new (app);
    gtk_window_set_title (GTK_WINDOW (window), "PSP awesome GUI");
    gtk_window_set_default_size (GTK_WINDOW (window), 400, 200);

    GtkWidget* grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 6);
    gtk_window_set_child(GTK_WINDOW(window), grid);

    widgetList = g_new(AppWidgets, 1);

    widgetList->statusBar = gtk_label_new("connectedState");
    gtk_grid_attach(GTK_GRID(grid), widgetList->statusBar, 2, 0, 6, 1);

//  The great button wall
    GdkRGBA initial_color;
    gdk_rgba_parse(&initial_color, "black");
    widgetList->lt = gtk_label_new("LT");
    //gtk_widget_override_color(buttonViewer, GTK_STATE_FLAG_NORMAL, &initial_color);
    gtk_grid_attach(GTK_GRID(grid), widgetList->lt, 0, 2, 2, 1);

    widgetList->rt = gtk_label_new("RT");
    //gtk_widget_override_color(buttonViewer, GTK_STATE_FLAG_NORMAL, &initial_color);
    gtk_grid_attach(GTK_GRID(grid), widgetList->rt, 8, 2, 2, 1);

    widgetList->lx = gtk_label_new("Lx:");
    gtk_grid_attach(GTK_GRID(grid), widgetList->lx, 2, 2, 2, 1);
    widgetList->lx = gtk_label_new("000");
    gtk_grid_attach(GTK_GRID(grid), widgetList->lxValue, 4, 2, 1, 1);
    widgetList->ly = gtk_label_new("Ly:");
    gtk_grid_attach(GTK_GRID(grid), widgetList->ly, 5, 2, 2, 1);
    widgetList->lyValue = gtk_label_new("000");
    gtk_grid_attach(GTK_GRID(grid), widgetList->lyValue, 7, 2, 1, 1);

    widgetList->left = gtk_label_new("←");
    gtk_grid_attach(GTK_GRID(grid), widgetList->left, 0, 3, 1, 1);
    widgetList->up = gtk_label_new("↑");
    gtk_grid_attach(GTK_GRID(grid), widgetList->up, 1, 3, 1, 1);
    widgetList->right = gtk_label_new("→");
    gtk_grid_attach(GTK_GRID(grid), widgetList->right, 0, 4, 1, 1);
    widgetList->down = gtk_label_new("↓");
    gtk_grid_attach(GTK_GRID(grid), widgetList->down, 1, 4, 1, 1);

    widgetList->home = gtk_label_new("Home");
    gtk_grid_attach(GTK_GRID(grid), widgetList->home, 2, 4, 1, 1);
    widgetList->hold = gtk_label_new("Hold");
    gtk_grid_attach(GTK_GRID(grid), widgetList->hold, 3, 4, 1, 1);
    widgetList->screen = gtk_label_new("Screen");
    gtk_grid_attach(GTK_GRID(grid), widgetList->screen, 4, 4, 1, 1);
    widgetList->note = gtk_label_new("Note");
    gtk_grid_attach(GTK_GRID(grid), widgetList->note, 5, 4, 1, 1);
    widgetList->select = gtk_label_new("Select");
    gtk_grid_attach(GTK_GRID(grid), widgetList->select, 6, 4, 1, 1);
    widgetList->start = gtk_label_new("Start");
    gtk_grid_attach(GTK_GRID(grid), widgetList->start, 7, 4, 1, 1);

    widgetList->square = gtk_label_new("□");
    gtk_grid_attach(GTK_GRID(grid), widgetList->square, 8, 3, 1, 1);
    widgetList->triangle = gtk_label_new("△");
    gtk_grid_attach(GTK_GRID(grid), widgetList->triangle, 9, 3, 1, 1);
    widgetList->circle = gtk_label_new("○");
    gtk_grid_attach(GTK_GRID(grid), widgetList->circle, 8, 4, 1, 1);
    widgetList->cross = gtk_label_new("X");
    gtk_grid_attach(GTK_GRID(grid), widgetList->cross, 9, 4, 1, 1);


// rest of the code
    // this is the "Close" button
    button = gtk_button_new_with_label("Evil");
    g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_destroy), window);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 7, 10, 1);

    gtk_window_present(GTK_WINDOW(window));

    //thread do stdin
    pthread_t thread;
    pthread_create(&thread, NULL, stdin_thread, NULL);
    pthread_detach(thread);
}

int main (int argc, char **argv) {
    GtkApplication *app;
    int status;

    app = gtk_application_new ("gtk.psp.gui", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
    status = g_application_run (G_APPLICATION (app), argc, argv);
    g_object_unref (app);

    return status;
}
