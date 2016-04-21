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
  
  //PID gain values
 uint8_t spin_x_p = 0;
 uint8_t spin_x_i = 0;
 uint8_t spin_x_d = 0;
 uint8_t spin_y_p = 0;
 uint8_t spin_y_i = 0;
 uint8_t spin_y_d = 0;
 uint8_t spin_z_p = 0;
 uint8_t spin_z_i = 0;
 
 char send_gain = 0;
 char gain_read_back = 0;
 char wait_count = 0;

 GtkWidget       *open_window;
 
struct _Widgets
{
   GtkEntry *e1;
   GtkEntry *e2;
   GtkSpinButton *sx_p;
   GtkSpinButton *sx_i;
   GtkSpinButton *sx_d;
   GtkSpinButton *sy_p;
   GtkSpinButton *sy_i;
   GtkSpinButton *sy_d;
   GtkSpinButton *sz_p;
   GtkSpinButton *sz_i;
   GtkAdjustment *a_x_p;
   GtkAdjustment *a_x_i;
   GtkAdjustment *a_x_d;
   GtkAdjustment *a_y_p;
   GtkAdjustment *a_y_i;
   GtkAdjustment *a_y_d;
   GtkAdjustment *a_z_p;
   GtkAdjustment *a_z_i;
   GtkScale *s4;
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
   GtkLabel *l16;
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
   int16_t chk_sum = 0;
   
   unsigned char x_scaled = (axis[0]/300)+100;
   //unsigned char x_send = (unsigned char)x_scaled;
   sprintf(buffer, "%d",x_com);
   gtk_label_set_label (widg->l5,buffer);
   sprintf(buffer, "%d",y_com);
   gtk_label_set_label (widg->l6,buffer);
   sprintf(buffer, "%d",t_com);
   gtk_label_set_label (widg->l7,buffer);
   sprintf(buffer, "%d",r_com);
   gtk_label_set_label (widg->l8,buffer);
   //count++;
   
  // int16_t x_com = axis[0]/100; 
   
   cnt++;
   if(cnt>254)cnt=0;
   
   if(send_gain == 1)
   {
   message[0] = 133;//preamble
   message[1] = 123;//preamble
   message[2] = 116;//preamble
   message[3] = 153;//preamble
   message[4] = spin_x_p;
   message[5] = spin_x_i;
   message[6] = spin_x_d;
   message[7] = spin_y_p;
   message[8] = spin_y_i;
   message[9] = spin_y_d;
   message[10] = spin_z_p;
   message[11] = spin_z_i;
   
   for(i = 4;i < 12;i++)
   {
     chk_sum += message[i];
   }
   
   message[12] = chk_sum & 0xFF;
   message[13] = chk_sum >> 8;
   
   send_gain = 0;
   }
   else
   {
      //sprintf(message,"count: %d", i);
   //message[0] = x_scaled;
   message[0] = 132;//preamble
   message[1] = 122;//preamble
   message[2] = 115;//preamble
   message[3] = 152;//preamble
   message[4] = x_com & 0xFF;//cnt;
   message[5] = x_com >> 8;
   message[6] = y_com & 0xFF;
   message[7] = y_com >> 8;
   message[8] = t_com & 0xFF;
   message[9] = t_com >> 8;
   message[10] = r_com & 0xFF;
   message[11] = r_com >> 8;
   
   for(i = 4;i < 12;i++)
   {
     chk_sum += message[i];
   }
   //printf("chk_sum: %d\n",chk_sum);
   message[12] = chk_sum & 0xFF;
   message[13] = chk_sum >> 8;
   }
        
   
        //Send some data
        if( send(sock , message , 14 , 0) < 0)//if( send(sock , message , strlen(message) , 0) < 0)
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
        
        if(gain_read_back == 1)
	{
	 if(server_reply[0]==spin_x_p&&server_reply[1]==spin_x_i&&server_reply[2]==spin_x_d&&server_reply[3]==spin_y_p
	   &&server_reply[4]==spin_y_i&&server_reply[5]==spin_y_d&&server_reply[6]==spin_z_p&&server_reply[7]==spin_z_i)
	 {
	   gtk_label_set_label (widg->l16,"read back OK");
	   gain_read_back = 0;
	 }
	 
	 printf("x_p: %d x_i: %d x_d: %d y_p: %d y_i: %d y_d: %d z_p: %d z_i: %d\n",server_reply[0],server_reply[1],server_reply[2],
	server_reply[3],server_reply[4],server_reply[5],server_reply[6],server_reply[7]);
	 
	 wait_count +=1;
	 if(wait_count > 5)
	 {
	   gtk_label_set_label (widg->l16,"read back failed");
	   gain_read_back = 0;
	   wait_count = 0;
	 }
	 
	}
        
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
    gtk_builder_add_from_file (builder, "Ground_station_2.glade", NULL);

    window = GTK_WIDGET(gtk_builder_get_object(builder, "window1"));
    open_window = GTK_WIDGET(gtk_builder_get_object(builder, "filechooserdialog1"));
    //widg.e1 = GTK_ENTRY(gtk_builder_get_object(builder, "entry1" )); 
    widg.sx_p = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonX_P" ));
    widg.sx_i = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonX_I" ));
    widg.sx_d = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonX_D" ));
    widg.sy_p = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonY_P" ));
    widg.sy_i = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonY_I" ));
    widg.sy_d = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonY_D" ));
    widg.sz_p = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonZ_P" ));
    widg.sz_i = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonZ_I" ));
    widg.a_x_p = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment1" ));
    widg.a_x_i = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment2" ));
    widg.a_x_d = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment3" ));
    widg.a_y_p = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment4" ));
    widg.a_y_i = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment5" ));
    widg.a_y_d = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment6" ));
    widg.a_z_p = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment7" ));
    widg.a_z_i = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment8" ));
    widg.s4 = GTK_SCALE(gtk_builder_get_object(builder, "scale2" )); 
    widg.l1 = GTK_LABEL(gtk_builder_get_object( builder, "label1" ));
    widg.l2 = GTK_LABEL(gtk_builder_get_object( builder, "label2" ));
    widg.l3 = GTK_LABEL(gtk_builder_get_object( builder, "label3" ));
    widg.l4 = GTK_LABEL(gtk_builder_get_object( builder, "label4" ));
    widg.l5 = GTK_LABEL(gtk_builder_get_object( builder, "label5" ));
    widg.l6 = GTK_LABEL(gtk_builder_get_object( builder, "label6" ));
    widg.l7 = GTK_LABEL(gtk_builder_get_object( builder, "label7" ));
    widg.l8 = GTK_LABEL(gtk_builder_get_object( builder, "label8" ));
    widg.l16 = GTK_LABEL(gtk_builder_get_object( builder, "label16" ));

    gtk_builder_connect_signals(builder, &widg);

    g_object_unref(builder);
    
    g_timeout_add(50, (GSourceFunc) time_handler, &widg);//was 100
    
    //widg.a_x = gtk_spin_button_get_adjustment (widg.sx_p);
    
    gtk_spin_button_configure (widg.sx_p,widg.a_x_p,1,0);
    gtk_spin_button_set_range(widg.sx_p,0,256);
    gtk_spin_button_set_update_policy (widg.sx_p,GTK_UPDATE_ALWAYS);
    gtk_spin_button_configure (widg.sx_i,widg.a_x_i,1,0);
    gtk_spin_button_set_range(widg.sx_i,0,256);
    gtk_spin_button_set_update_policy (widg.sx_i,GTK_UPDATE_ALWAYS);
    gtk_spin_button_configure (widg.sx_d,widg.a_x_d,1,0);
    gtk_spin_button_set_range(widg.sx_d,0,256);
    gtk_spin_button_set_update_policy (widg.sx_d,GTK_UPDATE_ALWAYS);
    
    gtk_spin_button_configure (widg.sy_p,widg.a_y_p,1,0);
    gtk_spin_button_set_range(widg.sy_p,0,256);
    gtk_spin_button_set_update_policy (widg.sy_p,GTK_UPDATE_ALWAYS);
    gtk_spin_button_configure (widg.sy_i,widg.a_y_i,1,0);
    gtk_spin_button_set_range(widg.sy_i,0,256);
    gtk_spin_button_set_update_policy (widg.sy_i,GTK_UPDATE_ALWAYS);
    gtk_spin_button_configure (widg.sy_d,widg.a_y_d,1,0);
    gtk_spin_button_set_range(widg.sy_d,0,256);
    gtk_spin_button_set_update_policy (widg.sy_d,GTK_UPDATE_ALWAYS);
    
     gtk_spin_button_configure (widg.sz_p,widg.a_z_p,1,0);
    gtk_spin_button_set_range(widg.sz_p,0,256);
    gtk_spin_button_set_update_policy (widg.sz_p,GTK_UPDATE_ALWAYS);
    gtk_spin_button_configure (widg.sz_i,widg.a_z_i,1,0);
    gtk_spin_button_set_range(widg.sz_i,0,256);
    gtk_spin_button_set_update_policy (widg.sz_i,GTK_UPDATE_ALWAYS);

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
void on_button1_clicked( GtkButton *button, Widgets *widg, gpointer window)
{
	//gtk_label_set_label (widg->l3,"test");
	//gtk_label_set_label (widg->l4,"test 2");
//	gint gtk_spin_button_get_value_as_int( GtkSpinButton *spin_button );
	gdouble spin_3 = gtk_spin_button_get_value (widg->sx_p);
	spin_x_p = gtk_spin_button_get_value (widg->sx_p);
	spin_x_i = gtk_spin_button_get_value (widg->sx_i);
	spin_x_d = gtk_spin_button_get_value (widg->sx_d);
	spin_y_p = gtk_spin_button_get_value (widg->sy_p);
	spin_y_i = gtk_spin_button_get_value (widg->sy_i);
	spin_y_d = gtk_spin_button_get_value (widg->sy_d);
	spin_z_p = gtk_spin_button_get_value (widg->sz_p);
	spin_z_i = gtk_spin_button_get_value (widg->sz_i);
	
	printf( "----------------------\n");
	printf( "spin x_p  %d\n",spin_x_p );
	printf( "spin x_i  %d\n",spin_x_i );
	printf( "spin x_d  %d\n",spin_x_d );
	printf( "spin y_p  %d\n",spin_y_p );
	printf( "spin y_i  %d\n",spin_y_i );
	printf( "spin y_d  %d\n",spin_y_d );
	printf( "spin z_p  %d\n",spin_z_p );
	printf( "spin z_i  %d\n",spin_z_i );
	
	send_gain = 1;
	gain_read_back = 1;//gain read back waiting state 1
	gtk_label_set_label (widg->l16,"waiting");
	
	/*
	
	FILE *fp;
   
        // open the file 
        fp = fopen("log.txt", "r+");
        if (fp == NULL) {
            printf("I couldn't open results.dat for appending.\n");
        }
        
        char str[8] = {spin_x_p,spin_x_i,spin_x_d,spin_y_p,spin_y_i,spin_y_d,spin_z_p,spin_z_i};
    
        // write to the file /
        //fprintf(fp, "------5-------------------------------------------------------\n");
	fwrite(str , 1 , sizeof(str) , fp );
        // close the file //
        fclose(fp);
	*/
	//gtk_widget_show(open_window);
	//GtkWidget *dialog;
        //dialog = gtk_file_chooser_dialog_new("Chosse a file", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
        //gtk_widget_show(dialog);
//      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog),"/");
        //gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dialog), g_get_home_dir());
        //gint resp = gtk_dialog_run(GTK_DIALOG(dialog));
        //if(resp == GTK_RESPONSE_OK)
          //      g_print("%s\n", gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
        //else
          //      g_print("You pressed Cancel\n");
        //gtk_widget_destroy(dialog);

}

void on_open_clicked(GtkButton *button, Widgets *widg, gpointer window)
{
  printf( "----------------------\n");
  printf("%s\n", gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_window)));
  gchar * filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(open_window));
  FILE *fp;
   
        /* open the file */
        fp = fopen(filename, "r+");
        if (fp == NULL) {
            printf("I couldn't open results.dat for appending.\n");
        }
        char buffer[8];
        fread(buffer, 8, 1, fp);
        gtk_spin_button_set_value (widg->sx_p,buffer[0]);
	gtk_spin_button_set_value (widg->sx_i,buffer[1]);
	gtk_spin_button_set_value (widg->sx_d,buffer[2]);
	gtk_spin_button_set_value (widg->sy_p,buffer[3]);
	gtk_spin_button_set_value (widg->sy_i,buffer[4]);
	gtk_spin_button_set_value (widg->sy_d,buffer[5]);
	gtk_spin_button_set_value (widg->sz_p,buffer[6]);
	gtk_spin_button_set_value (widg->sz_i,buffer[7]);
        /* close the file */
        fclose(fp);
  gtk_widget_hide(open_window);
  g_free(filename);
}

void on_save_gain_button_clicked(GtkButton *button, Widgets *widg, gpointer window)
{
        spin_x_p = gtk_spin_button_get_value (widg->sx_p);
	spin_x_i = gtk_spin_button_get_value (widg->sx_i);
	spin_x_d = gtk_spin_button_get_value (widg->sx_d);
	spin_y_p = gtk_spin_button_get_value (widg->sy_p);
	spin_y_i = gtk_spin_button_get_value (widg->sy_i);
	spin_y_d = gtk_spin_button_get_value (widg->sy_d);
	spin_z_p = gtk_spin_button_get_value (widg->sz_p);
	spin_z_i = gtk_spin_button_get_value (widg->sz_i);
	
	printf( "Saved----------------------\n");
	printf( "spin x_p  %d\n",spin_x_p );
	printf( "spin x_i  %d\n",spin_x_i );
	printf( "spin x_d  %d\n",spin_x_d );
	printf( "spin y_p  %d\n",spin_y_p );
	printf( "spin y_i  %d\n",spin_y_i );
	printf( "spin y_d  %d\n",spin_y_d );
	printf( "spin z_p  %d\n",spin_z_p );
	printf( "spin z_i  %d\n",spin_z_i );
	
	FILE *fp;
   
        /* open the file */
        fp = fopen("log.txt", "r+");
        if (fp == NULL) {
            printf("I couldn't open results.dat for appending.\n");
        }
        
        char str[8] = {spin_x_p,spin_x_i,spin_x_d,spin_y_p,spin_y_i,spin_y_d,spin_z_p,spin_z_i};
    
        /* write to the file */
        //fprintf(fp, "------5-------------------------------------------------------\n");
	fwrite(str , 1 , sizeof(str) , fp );
        /* close the file */
        fclose(fp);
	
	//gtk_widget_show(open_window);
}

void on_load_gain_button_clicked(GtkButton *button, Widgets *widg, gpointer window)
{
  gtk_widget_show(open_window);
}

void on_scale1_move_slider()
{
}
void on_imagemenuitem2_button_press_event()
{
  printf( "----------------------\n");
  gtk_widget_show(open_window);
}


