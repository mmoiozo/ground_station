#include <gtk/gtk.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>
#include <linux/joystick.h>
#include <fcntl.h>
#include <errno.h>
#include<sys/socket.h>    //socket
#include<arpa/inet.h> //inet_addr

#define JOY_DEV "/dev/input/js0"

typedef struct _Widgets Widgets;

int count = 0;
int i = 0;
int cnt = 0;

//joystick
int joy_fd, *axis=NULL, num_of_axis=0, num_of_buttons=0, x;
char *button=NULL, name_of_joystick[80];
struct js_event js;

  //SOCKET
  int sock;
  int flags;
  struct sockaddr_in server;
  unsigned char server_reply[2000];
  unsigned char message[1000];

struct _Widgets
{
   GtkEntry *e1;
   GtkEntry *e2;
   GtkLabel *l1;
   GtkLabel *l2;
   GtkLabel *l3;
   GtkLabel *l4;
   GtkLabel *l5;
   GtkLabel *l6;
   GtkLabel *l7;
   GtkLabel *l8;
   GtkLabel *l9;
   GtkLabel *l10;
};

gboolean time_handler(Widgets *widg) {
  
  /*
  int bytes = 1;

		while(bytes > 0) 	// infinite loop //
			{
			// read the joystick state //
		bytes = read(joy_fd, &js, sizeof(struct js_event));
		     }
			// see what to do with the event //
		switch (js.type & ~JS_EVENT_INIT)
		{
			case JS_EVENT_AXIS:
				axis   [ js.number ] = js.value;
				break;
			case JS_EVENT_BUTTON:
				button [ js.number ] = js.value;
				break;
		}
		*/
  
		   while (read(joy_fd, &js, sizeof(struct js_event)) == sizeof(struct js_event))  {
			  /*
				printf("Event: type %d, time %d, number %d, value %d\n",
					js.type, js.time, js.number, js.value); */
			  switch(js.type & ~JS_EVENT_INIT) {
			case JS_EVENT_BUTTON:
				button[js.number] = js.value;
				break;
			case JS_EVENT_AXIS:
				axis[js.number] = js.value;
				break;
			}
			/*
			printf("\r");

			if (num_of_axis) {
				printf("Axes: ");
				for (i = 0; i < num_of_axis; i++)
					printf("%2d:%6d ", i, axis[i]);
			}

			if (num_of_buttons) {
				printf("Buttons: ");
				for (i = 0; i < num_of_buttons; i++)
					printf("%2d:%s ", i, button[i] ? "on " : "off");
			}
*/
			fflush(stdout);
		   }
	
   char buffer [20];
   //count = axis[0];
   int16_t x_com = axis[0]/10; 
   int16_t y_com = axis[1]/10;
   int16_t t_com = axis[2]/10;
   int16_t r_com = axis[4]/10;
   
   unsigned char x_scaled = (axis[0]/300)+100;
   //unsigned char x_send = (unsigned char)x_scaled;
   sprintf(buffer, "%d",x_com);
   gtk_label_set_label (widg->l3,buffer);
   sprintf(buffer, "%d",y_com);
   gtk_label_set_label (widg->l4,buffer);
   sprintf(buffer, "%d",t_com);
   gtk_label_set_label (widg->l7,buffer);
   sprintf(buffer, "%d",r_com);
   gtk_label_set_label (widg->l8,buffer);
   //count++;
   
  // int16_t x_com = axis[0]/100; 
   
   cnt++;
   if(cnt>254)cnt=0;
   
   
      //sprintf(message,"count: %d", i);
   //message[0] = x_scaled;
   message[0] = 132;//preamble
   message[1] = 122;//preamble
   message[2] = 115;//preamble
   message[3] = 152;//preamble
   message[4] = cnt;//x_com & 0xFF;
   message[5] = x_com >> 8;
   message[6] = y_com & 0xFF;
   message[7] = y_com >> 8;
   message[8] = t_com & 0xFF;
   message[9] = t_com >> 8;
   message[10] = r_com & 0xFF;
   message[11] = r_com >> 8;
   
        
   
        //Send some data
        if( send(sock , message , 12 , 0) < 0)//if( send(sock , message , strlen(message) , 0) < 0)
        {
           // printf("Send failed");
            return 1;
        }
        usleep(10000);
	
	
        //Receive a reply from the server
        if( recv(sock , server_reply , 2000 , 0) < 0)
        {
            //printf("recv failed");
            //break;
        }
        
        
        //puts("Server reply :");
        //puts(server_reply);
        
        int16_t x_angle = (server_reply[1] << 8) | server_reply[0];
        int16_t y_angle = (server_reply[3] << 8) | server_reply[2];
	int16_t alt = (server_reply[5] << 8) | server_reply[4];
	int16_t loop_rate = (server_reply[7] << 8) | server_reply[6];
	int16_t connected = (server_reply[9] << 8) | server_reply[8];
        printf("x_angle: %d y_angle: %d altitude: %d loop_rate: %d connected %d\n",x_angle,y_angle,alt,loop_rate,connected);
   
  
  return TRUE;
}

int main(int argc, char *argv[])
{
  
     
    //Create socket
    sock = socket(AF_INET , SOCK_STREAM , 0);
    if (sock == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    /* Set socket to non-blocking */ 

    if ((flags = fcntl(sock, F_GETFL, 0)) < 0) 
    { 
      puts("F_GETFL error");
	/* Handle error */ 
    } 
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) 
    { 
      puts("F_GETFL error");
	/* Handle error */ 
    } 
    
    
    server.sin_addr.s_addr = inet_addr("192.168.4.1");
    server.sin_family = AF_INET;
    server.sin_port = htons( 80 );
 /*
    //Connect to remote server
    if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
    {
        perror("connect failed. Error");
        return 1;
    }
   */
 
  int res; 
  fd_set myset; 
  struct timeval tv; 
  int valopt; 
  socklen_t lon; 
 
 
 // Trying to connect with timeout 
  res = connect(sock, (struct sockaddr *)&server, sizeof(server)); 
  if (res < 0) { 
     if (errno == EINPROGRESS) { 
        fprintf(stderr, "EINPROGRESS in connect() - selecting\n"); 
        do { 
           tv.tv_sec = 15; 
           tv.tv_usec = 0; 
           FD_ZERO(&myset); 
           FD_SET(sock, &myset); 
           res = select(sock+1, NULL, &myset, NULL, &tv); 
           if (res < 0 && errno != EINTR) { 
              fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
              exit(0); 
           } 
           else if (res > 0) { 
              // Socket selected for write 
              lon = sizeof(int); 
              if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0) { 
                 fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno)); 
                 exit(0); 
              } 
              // Check the value returned... 
              if (valopt) { 
                 fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt) 
); 
                 exit(0); 
              } 
              break; 
           } 
           else { 
              fprintf(stderr, "Timeout in select() - Cancelling!\n"); 
              //exit(0);
	      break;
           } 
        } while (1); 
     } 
     else { 
        fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno)); 
        exit(0); 
     } 
  } 
  // Set to blocking mode again... 
    if ((flags = fcntl(sock, F_GETFL, 0)) < 0) 
    { 
      puts("F_GETFL error");
	/* Handle error */ 
    } 
    if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0) 
    { 
      puts("F_GETFL error");
	/* Handle error */ 
    } 
  

 
    puts("Connected\n");
     
    //int i = 0;
  //-------------------------------------------------------------------------------------
  
  //joystick


  if(( joy_fd = open( JOY_DEV , O_RDONLY)) == -1 )
  {
	  printf( "Couldn't open joystick\n" );
	  return -1;
  }

  ioctl( joy_fd, JSIOCGAXES, &num_of_axis );
  ioctl( joy_fd, JSIOCGBUTTONS, &num_of_buttons );
  ioctl( joy_fd, JSIOCGNAME(80), &name_of_joystick );

  axis = (int *) calloc( num_of_axis, sizeof( int ) );
  button = (char *) calloc( num_of_buttons, sizeof( char ) );

  printf("Joystick detected: %s\n\t%d axis\n\t%d buttons\n\n"
	  , name_of_joystick
	  , num_of_axis
	  , num_of_buttons );

  fcntl( joy_fd, F_SETFL, O_NONBLOCK );	/* use non-blocking mode */

  
  
    GtkBuilder      *builder; 
    GtkWidget       *window;
    Widgets     widg;

    gtk_init(&argc, &argv);

    builder = gtk_builder_new();
    gtk_builder_add_from_file (builder, "Ground_station_1.glade", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    //widg.e1 = GTK_ENTRY(gtk_builder_get_object(builder, "entry1" )); 
    widg.l1 = GTK_LABEL(gtk_builder_get_object( builder, "label1" ));
    widg.l2 = GTK_LABEL(gtk_builder_get_object( builder, "label2" ));
    widg.l3 = GTK_LABEL(gtk_builder_get_object( builder, "label3" ));
    widg.l4 = GTK_LABEL(gtk_builder_get_object( builder, "label4" ));
    widg.l7 = GTK_LABEL(gtk_builder_get_object( builder, "label7" ));
    widg.l8 = GTK_LABEL(gtk_builder_get_object( builder, "label8" ));

    gtk_builder_connect_signals(builder, &widg);

    g_object_unref(builder);
    
    g_timeout_add(50, (GSourceFunc) time_handler, &widg);//was 100

    gtk_widget_show(window);
    gtk_main();

    return 0;
}

// called when window is closed
void on_window1_destroy()
{
    gtk_main_quit();
}

//button
void on_button1_clicked( GtkButton *button, Widgets *widg)
{
	gtk_label_set_label (widg->l3,"test");
	gtk_label_set_label (widg->l4,"test 2");
}

