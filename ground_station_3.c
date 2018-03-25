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
#include <sys/socket.h> //socket
#include <arpa/inet.h>  //inet_addr
#include <netinet/in.h>
#include <sys/types.h>
#include <netdb.h>

#define JOY_DEV "/dev/input/js0"

typedef struct _Widgets Widgets;

int count = 0;
int i = 0;
int cnt = 0;

//joystick
int joy_fd, *axis = NULL, num_of_axis = 0, num_of_buttons = 0, x;
char *button = NULL, name_of_joystick[80];
struct js_event js;

//SOCKET
int sock;
int flags;
struct sockaddr_in server;
unsigned char server_reply[2000];
unsigned char message[1000];

struct sockaddr_in server;
int len = sizeof(struct sockaddr_in);
struct hostent *host;
int n, s, port;
    #define BUF_SIZE 1024
uint8_t buf[BUF_SIZE];

int reconnecting = 0;
int recv_fail_count = 0;

void socket_reconnect();

//PID gain values
uint8_t spin_x_p = 0;
uint8_t spin_x_i = 0;
uint8_t spin_x_d = 0;
uint8_t spin_y_p = 0;
uint8_t spin_y_i = 0;
uint8_t spin_y_d = 0;
uint8_t spin_z_p = 0;
uint8_t spin_z_i = 0;
uint8_t spin_x_p_o = 0;
uint8_t spin_y_p_o = 0;

//trim values
uint8_t spin_pitch_trim = 0;
uint8_t spin_roll_trim = 0;

char send_gain = 0;
char gain_read_back = 0;
char wait_count = 0;

//Recording control
int rec_state_change = 0;
int rec_count = 0;
int rec_state = 0;
char rec_com = 0;

const uint8_t CRC7_POLY = 0x91;

uint8_t getCRC(uint8_t message[], int length)
{
  uint8_t i, j, crc = 0;

  for (i = 0; i < length; i++)
  {
    crc ^= message[i];
    for (j = 0; j < 8; j++)
    {
      if (crc & 1)
        crc ^= CRC7_POLY;
      crc >>= 1;
    }
  }
  return crc;
}

GtkWidget       *open_window;

struct _Widgets {
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
	GtkSpinButton *sx_p_o;
	GtkSpinButton *sy_p_o;
	GtkSpinButton *s_pitch_trim;
	GtkSpinButton *s_roll_trim;
	GtkAdjustment *a_x_p;
	GtkAdjustment *a_x_i;
	GtkAdjustment *a_x_d;
	GtkAdjustment *a_y_p;
	GtkAdjustment *a_y_i;
	GtkAdjustment *a_y_d;
	GtkAdjustment *a_z_p;
	GtkAdjustment *a_z_i;
	GtkAdjustment *a_x_p_o;
	GtkAdjustment *a_y_p_o;
	GtkAdjustment *a_pitch_trim;
	GtkAdjustment *a_roll_trim;
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

gboolean time_handler(Widgets *widg)
{

	/*
	   int bytes = 1;

	              while(bytes > 0)  // infinite loop //
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

	while (read(joy_fd, &js, sizeof(struct js_event)) == sizeof(struct js_event)) {
		/*
		      printf("Event: type %d, time %d, number %d, value %d\n",
		              js.type, js.time, js.number, js.value); */
		switch (js.type & ~JS_EVENT_INIT) {
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
	int16_t x_com = axis[0];
	int16_t y_com = axis[1];
	int16_t t_com = axis[2];
	int16_t r_com = axis[4];
	int16_t chk_sum = 0;

	//unsigned char x_scaled = (axis[0]/300)+100;
	//unsigned char x_send = (unsigned char)x_scaled;
	sprintf(buffer, "%d", x_com);
	gtk_label_set_label(widg->l5, buffer);
	sprintf(buffer, "%d", y_com);
	gtk_label_set_label(widg->l6, buffer);
	sprintf(buffer, "%d", t_com);
	gtk_label_set_label(widg->l7, buffer);
	sprintf(buffer, "%d", r_com);
	gtk_label_set_label(widg->l8, buffer);
	//count++;

	// int16_t x_com = axis[0]/100;

	//optional package increment counter
	cnt++;
	if (cnt > 254) cnt = 0;

	//recording control counter
	if (rec_com != 0 && rec_count < 5) rec_count++;
	else if (rec_count >= 5) {
		rec_count = 0;
		rec_com = 0;
	}


	if (send_gain == 1) {
		message[0] = 133;       //preamble
		message[1] = 123;       //preamble
		message[2] = 116;       //preamble
		message[3] = 153;       //preamble
		message[4] = spin_x_p;
		message[5] = spin_x_i;
		message[6] = spin_x_d;
		message[7] = spin_y_p;
		message[8] = spin_y_i;
		message[9] = spin_y_d;
		message[10] = spin_z_p;
		message[11] = spin_z_i;
		message[12] = spin_x_p_o;
		message[13] = spin_y_p_o;
		message[14] = spin_pitch_trim;
		message[15] = spin_roll_trim;

		for (i = 4; i < 16; i++)
			chk_sum += message[i];

		message[16] = chk_sum & 0xFF;
		message[17] = chk_sum >> 8;

		send_gain = 0;

		/* send message */
		if (sendto(s, message, 18, 0, (struct sockaddr*)&server, len) == -1) {
			perror("sendto()");
			return -1;
		}

	}else  {

		int32_t out_buffer[7];
		out_buffer[0] = cnt;
		out_buffer[1] = 3392;
		out_buffer[2] = x_com;  //x_com;
		out_buffer[3] = y_com;  //y_com;
		out_buffer[4] = r_com;  //r_com;
		out_buffer[5] = t_com;
        memcpy(message, out_buffer, 6*4);
        uint8_t checksum_crc = getCRC(message,6*4);
        //printf("checksum_crc:%d\n",checksum_crc);
        out_buffer[6] = (int32_t)checksum_crc;
		// out_buffer[6] = -142;

		int send_length = 7 * sizeof(int32_t);
		//printf("send_length%d\n",send_length);
		memcpy(message, out_buffer, send_length);

		//Send the data
		/* send message */
		if (sendto(s, message, send_length, 0, (struct sockaddr*)&server, len) == -1) {
			perror("sendto()");
			printf("Send failed");
			recv_fail_count += 1;
			return 1;
		}

	}

	usleep(10000);


	//Receive a reply from the server
	// if( recv(sock , server_reply , 2000 , 0) < 0)
	// {
	//     printf("recv failed");
	// recv_fail_count +=1;
	//     //break;
	// }
	int32_t in_buffer[30];
	n = recvfrom(s, server_reply, BUF_SIZE, 0, (struct sockaddr*)&server, &len);
    //printf("n:%d\n"n);
	if (n > 0) {
		// printf("Received from %s:%d: \n",
		//        inet_ntoa(server.sin_addr),
		//        ntohs(server.sin_port));

		//fflush(stdout);
		// write(1, buf, n);
		// write(1, "\n", 1);

		//convert from uint8_t to int32_t
		memcpy(in_buffer, server_reply, 120);

		//printf("echo_count[0]=%d\n", in_buffer[0]);
		//break;
		recv_fail_count = 0;
	}else
		recv_fail_count++;

	if (reconnecting == 0 && recv_fail_count > 40)
		//socket_reconnect();###########################################################################################
		recv_fail_count = 0;
	 //puts("Server reply :");
	//puts(server_reply);

	if (gain_read_back == 1) {
		if (server_reply[0] == spin_x_p && server_reply[1] == spin_x_i && server_reply[2] == spin_x_d && server_reply[3] == spin_y_p
		    && server_reply[4] == spin_y_i && server_reply[5] == spin_y_d && server_reply[6] == spin_z_p && server_reply[7] == spin_z_i
		    && server_reply[8] == spin_x_p_o && server_reply[9] == spin_y_p_o && server_reply[10] == spin_pitch_trim && server_reply[11] == spin_roll_trim) {
			gtk_label_set_label(widg->l16, "read back OK");
			gain_read_back = 0;
		}

		printf("x_p: %d x_i: %d x_d: %d y_p: %d y_i: %d y_d: %d z_p: %d z_i: %d x_p_o: %d y_p_o: %d\n", server_reply[0], server_reply[1], server_reply[2],
		       server_reply[3], server_reply[4], server_reply[5], server_reply[6], server_reply[7], server_reply[8], server_reply[9]);

		wait_count += 1;
		if (wait_count > 5) {
			gtk_label_set_label(widg->l16, "read back failed");
			gain_read_back = 0;
			wait_count = 0;
		}

	}

	// int16_t x_angle = (server_reply[1] << 8) | server_reply[0];
	// int16_t y_angle = (server_reply[3] << 8) | server_reply[2];
	// int16_t alt = (server_reply[5] << 8) | server_reply[4];
	// int16_t loop_rate = (server_reply[7] << 8) | server_reply[6];
	// int16_t connected = (server_reply[9] << 8) | server_reply[8];
	//printf("x_angle: %d y_angle: %d altitude: %d loop_rate: %d connected %d recording control: %d\n", x_angle, y_angle, alt, loop_rate, connected, rec_com);

    float send_factor = 100000;
    if(in_buffer[29]<255){
        float Angle_x = ((float)in_buffer[0]) / send_factor;
    	float Angle_y = ((float)in_buffer[1]) / send_factor;
    	float Az = ((float)in_buffer[2]) / send_factor;
    	float p = ((float)in_buffer[3]) / send_factor;
    	float q = ((float)in_buffer[4]) / send_factor;
    	float r = ((float)in_buffer[5]) / send_factor;
    	float temp = ((float)in_buffer[6]) / send_factor;

    	float debug_apps_1 = ((float)in_buffer[7]) / send_factor;
    	float debug_apps_2 = ((float)in_buffer[8]) / send_factor;
    	float debug_apps_3 = ((float)in_buffer[9]) / send_factor;
    	float debug_apps_4 = ((float)in_buffer[10]) / send_factor;
    	float debug_apps_5 = ((float)in_buffer[11]) / send_factor;
    	float debug_apps_6 = ((float)in_buffer[12]) / send_factor;
    	float debug_apps_7 = ((float)in_buffer[13]) / send_factor;
    	float debug_apps_8 = ((float)in_buffer[14]) / send_factor;
    	float debug_apps_9 = ((float)in_buffer[15]) / send_factor;
    	float debug_apps_10 = ((float)in_buffer[16]) / send_factor;
        float debug_apps_11 = ((float)in_buffer[17]) / send_factor;
    	float debug_apps_12 = ((float)in_buffer[18]) / send_factor;
    	float debug_apps_13 = ((float)in_buffer[19]) / send_factor;
    	float debug_apps_14 = ((float)in_buffer[20]) / send_factor;
    	float debug_apps_15 = ((float)in_buffer[21]) / send_factor;
    	float debug_apps_16 = ((float)in_buffer[22]) / send_factor;
    	float debug_apps_17 = ((float)in_buffer[23]) / send_factor;
    	float debug_apps_18 = ((float)in_buffer[24]) / send_factor;
    	float debug_apps_19 = ((float)in_buffer[25]) / send_factor;
    	float debug_apps_20 = ((float)in_buffer[26]) / send_factor;

        //printf("Angle_x:%f Angle_y:%f \n",Angle_x, Angle_y);
        printf("--------------------------------------------------------------------\n");
        printf("cmd_Pitch:%f\natt_Theta:%f\ncmd_Roll:%f\natt_Phi:%f\ni_cmd_Pitch:%f\ni_cmd_roll:%f\nThrottle:%f\nPitch_act:%f\nRoll_act:%f\nfail_Safe:%f\n",
        debug_apps_1*57.3,debug_apps_2*57.3,debug_apps_3*57.3,debug_apps_4*57.3,debug_apps_5,debug_apps_6,debug_apps_7,debug_apps_8,debug_apps_9,debug_apps_10);
        printf("acc_filter_x_70:%f\nacc_filter_y_70:%f\nacc_filter_z_70:%f\n",debug_apps_11,debug_apps_12,debug_apps_13);
        printf("acc_filter_x_90:%f\nacc_filter_y_90:%f\nacc_filter_z_90:%f\n",debug_apps_14,debug_apps_15,debug_apps_16);
    }
	return TRUE;
}

int main(int argc, char *argv[])
{


	//Create socket
	// if (argc < 3) {
	// fprintf(stderr, "Usage: %s <host> <port>\n", argv[0]);
	// return 1;
	// }

	host = gethostbyname("192.168.1.1");
	if (host == NULL) {
		perror("gethostbyname");
		return 1;
	}

	port = 5004; //atoi(argv[2]);

	/* initialize socket */
	if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
		perror("socket");
		return 1;
	}

	/* initialize server addr */
	memset((char*)&server, 0, sizeof(struct sockaddr_in));
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	server.sin_addr = *((struct in_addr*)host->h_addr);

	fcntl(s, F_SETFL, O_NONBLOCK);


	puts("Connected\n");

	//int i = 0;
	//-------------------------------------------------------------------------------------

	//joystick


	if (( joy_fd = open( JOY_DEV, O_RDONLY)) == -1 ) {
		printf( "Couldn't open joystick\n" );
		//return -1;
	}

	ioctl( joy_fd, JSIOCGAXES, &num_of_axis );
	ioctl( joy_fd, JSIOCGBUTTONS, &num_of_buttons );
	ioctl( joy_fd, JSIOCGNAME(80), &name_of_joystick );

	axis = (int*)calloc( num_of_axis, sizeof( int ) );
	button = (char*)calloc( num_of_buttons, sizeof( char ) );

	printf("Joystick detected: %s\n\t%d axis\n\t%d buttons\n\n"
	       , name_of_joystick
	       , num_of_axis
	       , num_of_buttons );

	fcntl( joy_fd, F_SETFL, O_NONBLOCK ); /* use non-blocking mode */



	GtkBuilder      *builder;
	GtkWidget       *window;
	Widgets widg;

	gtk_init(&argc, &argv);

	builder = gtk_builder_new();
	gtk_builder_add_from_file(builder, "Ground_station_2.glade", NULL);

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
	widg.sx_p_o = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonX_P_outer" ));
	widg.sy_p_o = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbuttonY_P_outer" ));
	widg.s_pitch_trim = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbutton_Pitch_trim" ));
	widg.s_roll_trim = GTK_SPIN_BUTTON(gtk_builder_get_object(builder, "spinbutton_Roll_trim" ));

	widg.a_x_p = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment1" ));
	widg.a_x_i = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment2" ));
	widg.a_x_d = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment3" ));
	widg.a_y_p = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment4" ));
	widg.a_y_i = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment5" ));
	widg.a_y_d = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment6" ));
	widg.a_z_p = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment7" ));
	widg.a_z_i = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment8" ));
	widg.a_x_p_o = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment9" ));
	widg.a_y_p_o = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment10" ));
	widg.a_pitch_trim = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment11" ));
	widg.a_roll_trim = GTK_ADJUSTMENT(gtk_builder_get_object(builder, "adjustment12" ));

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

	g_timeout_add(50, (GSourceFunc)time_handler, &widg); //was 100

	//widg.a_x = gtk_spin_button_get_adjustment (widg.sx_p);

	gtk_spin_button_configure(widg.sx_p, widg.a_x_p, 1, 0);
	gtk_spin_button_set_range(widg.sx_p, 0, 256);
	gtk_spin_button_set_update_policy(widg.sx_p, GTK_UPDATE_ALWAYS);
	gtk_spin_button_configure(widg.sx_i, widg.a_x_i, 1, 0);
	gtk_spin_button_set_range(widg.sx_i, 0, 256);
	gtk_spin_button_set_update_policy(widg.sx_i, GTK_UPDATE_ALWAYS);
	gtk_spin_button_configure(widg.sx_d, widg.a_x_d, 1, 0);
	gtk_spin_button_set_range(widg.sx_d, 0, 256);
	gtk_spin_button_set_update_policy(widg.sx_d, GTK_UPDATE_ALWAYS);

	gtk_spin_button_configure(widg.sy_p, widg.a_y_p, 1, 0);
	gtk_spin_button_set_range(widg.sy_p, 0, 256);
	gtk_spin_button_set_update_policy(widg.sy_p, GTK_UPDATE_ALWAYS);
	gtk_spin_button_configure(widg.sy_i, widg.a_y_i, 1, 0);
	gtk_spin_button_set_range(widg.sy_i, 0, 256);
	gtk_spin_button_set_update_policy(widg.sy_i, GTK_UPDATE_ALWAYS);
	gtk_spin_button_configure(widg.sy_d, widg.a_y_d, 1, 0);
	gtk_spin_button_set_range(widg.sy_d, 0, 256);
	gtk_spin_button_set_update_policy(widg.sy_d, GTK_UPDATE_ALWAYS);

	gtk_spin_button_configure(widg.sz_p, widg.a_z_p, 1, 0);
	gtk_spin_button_set_range(widg.sz_p, 0, 256);
	gtk_spin_button_set_update_policy(widg.sz_p, GTK_UPDATE_ALWAYS);
	gtk_spin_button_configure(widg.sz_i, widg.a_z_i, 1, 0);
	gtk_spin_button_set_range(widg.sz_i, 0, 256);
	gtk_spin_button_set_update_policy(widg.sz_i, GTK_UPDATE_ALWAYS);

	gtk_spin_button_configure(widg.sx_p_o, widg.a_x_p_o, 1, 0);
	gtk_spin_button_set_range(widg.sx_p_o, 0, 256);
	gtk_spin_button_set_update_policy(widg.sx_p_o, GTK_UPDATE_ALWAYS);
	gtk_spin_button_configure(widg.sy_p_o, widg.a_y_p_o, 1, 0);
	gtk_spin_button_set_range(widg.sy_p_o, 0, 256);
	gtk_spin_button_set_update_policy(widg.sy_p_o, GTK_UPDATE_ALWAYS);

	gtk_spin_button_configure(widg.s_pitch_trim, widg.a_pitch_trim, 0.1, 2);
	gtk_spin_button_set_range(widg.s_pitch_trim, -20.0, 20.0);
	gtk_spin_button_set_update_policy(widg.s_pitch_trim, GTK_UPDATE_ALWAYS);
    //gtk_spin_button_set_increments (widg.s_pitch_trim,0.1,0.1);
	gtk_spin_button_configure(widg.s_roll_trim, widg.a_roll_trim, 0.1, 2);
	gtk_spin_button_set_range(widg.s_roll_trim, -20, 20);
	gtk_spin_button_set_update_policy(widg.s_roll_trim, GTK_UPDATE_ALWAYS);


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
	//gdouble spin_3 = gtk_spin_button_get_value (widg->sx_p);
	spin_x_p = gtk_spin_button_get_value(widg->sx_p);
	spin_x_i = gtk_spin_button_get_value(widg->sx_i);
	spin_x_d = gtk_spin_button_get_value(widg->sx_d);
	spin_y_p = gtk_spin_button_get_value(widg->sy_p);
	spin_y_i = gtk_spin_button_get_value(widg->sy_i);
	spin_y_d = gtk_spin_button_get_value(widg->sy_d);
	spin_z_p = gtk_spin_button_get_value(widg->sz_p);
	spin_z_i = gtk_spin_button_get_value(widg->sz_i);
	spin_x_p_o = gtk_spin_button_get_value(widg->sx_p_o);
	spin_y_p_o = gtk_spin_button_get_value(widg->sy_p_o);
	spin_pitch_trim = gtk_spin_button_get_value(widg->s_pitch_trim);
	spin_roll_trim = gtk_spin_button_get_value(widg->s_roll_trim);

	printf( "----------------------\n");
	printf( "spin x_p  %d\n", spin_x_p );
	printf( "spin x_i  %d\n", spin_x_i );
	printf( "spin x_d  %d\n", spin_x_d );
	printf( "spin y_p  %d\n", spin_y_p );
	printf( "spin y_i  %d\n", spin_y_i );
	printf( "spin y_d  %d\n", spin_y_d );
	printf( "spin z_p  %d\n", spin_z_p );
	printf( "spin z_i  %d\n", spin_z_i );
	printf( "spin x_p_o  %d\n", spin_x_p_o );
	printf( "spin y_p_o  %d\n", spin_y_p_o );
	printf( "spin pitch_trim  %d\n", spin_pitch_trim );
	printf( "spin roll_trim  %d\n", spin_roll_trim );

	send_gain = 1;
	gain_read_back = 1; //gain read back waiting state 1
	gtk_label_set_label(widg->l16, "waiting");

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
	if (fp == NULL)
		printf("I couldn't open results.dat for appending.\n");
	uint8_t buffer[12];
	fread(buffer, 12, 1, fp);
	gtk_spin_button_set_value(widg->sx_p, buffer[0]);
	gtk_spin_button_set_value(widg->sx_i, buffer[1]);
	gtk_spin_button_set_value(widg->sx_d, buffer[2]);
	gtk_spin_button_set_value(widg->sy_p, buffer[3]);
	gtk_spin_button_set_value(widg->sy_i, buffer[4]);
	gtk_spin_button_set_value(widg->sy_d, buffer[5]);
	gtk_spin_button_set_value(widg->sz_p, buffer[6]);
	gtk_spin_button_set_value(widg->sz_i, buffer[7]);
	gtk_spin_button_set_value(widg->sx_p_o, buffer[8]);
	gtk_spin_button_set_value(widg->sy_p_o, buffer[9]);
	gtk_spin_button_set_value(widg->s_pitch_trim, buffer[10]);
	gtk_spin_button_set_value(widg->s_roll_trim, buffer[11]);
	/* close the file */
	fclose(fp);
	gtk_widget_hide(open_window);
	g_free(filename);
}

void on_save_gain_button_clicked(GtkButton *button, Widgets *widg, gpointer window)
{
	spin_x_p = gtk_spin_button_get_value(widg->sx_p);
	spin_x_i = gtk_spin_button_get_value(widg->sx_i);
	spin_x_d = gtk_spin_button_get_value(widg->sx_d);
	spin_y_p = gtk_spin_button_get_value(widg->sy_p);
	spin_y_i = gtk_spin_button_get_value(widg->sy_i);
	spin_y_d = gtk_spin_button_get_value(widg->sy_d);
	spin_z_p = gtk_spin_button_get_value(widg->sz_p);
	spin_z_i = gtk_spin_button_get_value(widg->sz_i);
	spin_x_p_o = gtk_spin_button_get_value(widg->sx_p_o);
	spin_y_p_o = gtk_spin_button_get_value(widg->sy_p_o);
	spin_pitch_trim = gtk_spin_button_get_value(widg->s_pitch_trim);
	spin_roll_trim = gtk_spin_button_get_value(widg->s_roll_trim);

	printf( "Saved----------------------\n");
	printf( "spin x_p  %d\n", spin_x_p );
	printf( "spin x_i  %d\n", spin_x_i );
	printf( "spin x_d  %d\n", spin_x_d );
	printf( "spin y_p  %d\n", spin_y_p );
	printf( "spin y_i  %d\n", spin_y_i );
	printf( "spin y_d  %d\n", spin_y_d );
	printf( "spin z_p  %d\n", spin_z_p );
	printf( "spin z_i  %d\n", spin_z_i );
	printf( "spin x_p_o  %d\n", spin_x_p_o );
	printf( "spin y_p_o  %d\n", spin_y_p_o );
	printf( "spin pitch_trim  %d\n", spin_pitch_trim );
	printf( "spin roll_trim  %d\n", spin_roll_trim );

	FILE *fp;

	/* open the file */
	fp = fopen("log.txt", "r+");
	if (fp == NULL)
		printf("I couldn't open results.dat for appending.\n");

	uint8_t str[12] = { spin_x_p, spin_x_i, spin_x_d, spin_y_p, spin_y_i, spin_y_d, spin_z_p, spin_z_i, spin_x_p_o, spin_y_p_o, spin_pitch_trim, spin_roll_trim };

	/* write to the file */
	//fprintf(fp, "------5-------------------------------------------------------\n");
	fwrite(str, 1, sizeof(str), fp );
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

void on_start_recording_button_clicked()
{
	if (rec_com == 0) {
		rec_com = 1; //start recording
		printf( "START----------------------\n");
	}
}
void on_stop_recording_button_clicked()
{
	if (rec_com == 0) {
		rec_com = 2; //stop recording
		printf( "STOP----------------------\n");
	}
}

void socket_reconnect()
{
	int res;
	fd_set myset;
	struct timeval tv;
	int valopt;
	socklen_t lon;

	printf( "RECONNECTING----------------------\n");

	reconnecting = 1;
	//close existing socket
	close(sock);

	//Create socket
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == -1)
		printf("Could not create socket");
	puts("Socket created");

	/* Set socket to non-blocking */

	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		puts("F_GETFL error");
		/* Handle error */
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
		puts("F_GETFL error");
		/* Handle error */


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



	// Trying to connect with timeout
	res = connect(sock, (struct sockaddr*)&server, sizeof(server));
	if (res < 0) {
		if (errno == EINPROGRESS) {
			fprintf(stderr, "EINPROGRESS in connect() - selecting\n");
			do {
				tv.tv_sec = 3;
				tv.tv_usec = 0;
				FD_ZERO(&myset);
				FD_SET(sock, &myset);
				res = select(sock + 1, NULL, &myset, NULL, &tv);
				if (res < 0 && errno != EINTR)
					fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
					//exit(0);
				else if (res > 0) {
					// Socket selected for write
					lon = sizeof(int);
					if (getsockopt(sock, SOL_SOCKET, SO_ERROR, (void*)(&valopt), &lon) < 0)
						fprintf(stderr, "Error in getsockopt() %d - %s\n", errno, strerror(errno));
						// exit(0);
					// Check the value returned...
					if (valopt) {
						fprintf(stderr, "Error in delayed connection() %d - %s\n", valopt, strerror(valopt)
							);
						//exit(0);
					}
					break;
				}else  {
					fprintf(stderr, "Timeout in select() - Cancelling!\n");
					//exit(0);
					break;
				}
			} while (1);
		}else
			fprintf(stderr, "Error connecting %d - %s\n", errno, strerror(errno));
			//exit(0);
	}
	// Set to blocking mode again...
	if ((flags = fcntl(sock, F_GETFL, 0)) < 0)
		puts("F_GETFL error");
		/* Handle error */
	if (fcntl(sock, F_SETFL, flags | O_NONBLOCK) < 0)
		puts("F_GETFL error");
		/* Handle error */

	puts("Connected\n");
	reconnecting = 0;

}
