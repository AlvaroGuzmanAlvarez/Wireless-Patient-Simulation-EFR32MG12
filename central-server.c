/*Central server code*/

//Includes

#include "contiki.h"

#include <stdio.h> /* For printf() */
#include "leds.h"

#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "net/ipv6/uiplib.h"

#include "serial-line.h"
#include "string.h"

#include "efr32mg12p432f1024gl125.h"
#include "em_cmu.h"
#include "em_emu.h"
#include "si7021.h"
#include "board_cookie.h"
#include "util.h"
#include "imu.h" // For IMU
#include "icm20648.h" // For IMU

/*-------------------------------Defines--------------------------------------*/

//Port defines
#define UDP_CLIENT_PORT 8765
#define UDP_ROOM_2_PORT 8766
#define UDP_PATIENT_ACC 8767
#define UDP_PATIENT_HRT 8768

#define UDP_SERVER_PORT 5678

//Message defines
#define MY_MESSAGE "Message received"

/*--------------------------Global variables-----------------------------------*/

//UDP connection variables
static struct simple_udp_connection udp_conn;
static struct simple_udp_connection room2_conn;
static struct simple_udp_connection acc_conn;
static struct simple_udp_connection hrt_snsr_conn;

//Measurement variables
	//Acceleration
float acceleration = 0;
int accel = 0;
	//Humidity
float hume_r1 = 0; 
float hume_r2 = 0;
    //Temperature 
float temper_r1 = 0; 
float temper_r2 = 0; 
    //Heartbeat ratio 
int hrtbt = 0; 

//Alarm variables
uint8_t fall_alarm = 0;
uint8_t heartbeat_alarm = 0; //Different heartbeat -> different room 
uint8_t critical_hrtbt = 0; //Critical heartbeat values 
uint8_t room1_alarm = 0; //Alarm for humidity or temperature in room 1

//Alarm counters 
int fall_alarm_counter = 0; 
int hrtbt_alarm_counter = 0; 
int chrtbt_alarm_counter = 0; 
int room_1_alarm_counter = 0;

/*---------------------------------------------------------------------------*/

void udp_rx_callback(struct simple_udp_connection *c,
                                     const uip_ipaddr_t *source_addr,
                                     uint16_t source_port,
                                     const uip_ipaddr_t *dest_addr,
                                     uint16_t dest_port,
                                     const uint8_t *data, uint16_t datalen){
	printf("Received Packet from: ");
	uiplib_ipaddr_print(source_addr);
	printf("\n");
	printf("%s\n", (char*)data);

	//See which measurement we have received

	if(strncmp((char*)data, "AC", 2) == 0){//Acceleration
		accel = atoi((char*)data + 3);
		acceleration = accel/1000;

		if(acceleration > 1500.0){
			fall_alarm = 1;
		}
	}

	 else if(strncmp((char*)data, "R1H", 3) == 0){//Room 1 Humidity
		//printf("Humidity from room 1: %s\n", (char*)data); 
        hume_r1 = atoi((char*) data + 4)/1000; 
        if(hume_r1 > 60.0){//More than 60.0% in Humidity is not acceptable 
            room1_alarm = 1; 
        } 
 
    } 

	else if(strncmp((char*)data, "R1T", 3) == 0){//Room 1 Temperature 
 
            //printf("Temperature from room 1: %s\n", (char*)data); 
            temper_r1 = atoi((char*) data + 4)/1000; 
            if(temper_r1 > 30.0 || temper_r1 < 20.0){//More than 30 degrees or less than 20 degrees is considered abnormal temperature 
                room1_alarm = 1; 
            } 
    } 

	else if(strncmp((char*)data, "R2H", 3) == 0){//Room 2 Humidity 
 
        //printf("Humidity from room 2: %s\n", (char*)data); 
        hume_r2 = atoi((char*) data + 4)/1000; 
        if(hume_r2 > 60.0){//More than 60.0% in Humidity is not acceptable 
            room1_alarm = 1; 
        } 
    }

	 else if(strncmp((char*)data, "R2T", 3) == 0){//Room 2 Temperature 
 
        //printf("Temperature from room 2: %s\n", (char*)data); 
        temper_r2 = atoi((char*) data + 4)/1000;
		if(temper_r2 > 45.0 || temper_r2 < 20.0){//More than 30 degrees or less than 20 degrees is considered abnormal temperature 
            room1_alarm = 1; 
        } 
    }

	else if(strncmp((char*)data, "HS", 2) == 0){//Heartbeat sensor 
        hrtbt = atoi((char*)data + 3); 
        //Case where the patient can be in other room or in heartbeat critical conditions 
        if(hrtbt >= 100 || hrtbt <= 60){ 
            //Heartbeat associated to be in other room with different temperature 
            if(hrtbt >= 100 && hrtbt <= 120){ 
                heartbeat_alarm = 1; 
            } 
            else critical_hrtbt = 1; 
        } 
    }
}

/*---------------------------------------------------------------------------*/
PROCESS(p3_udp_server_process, "UDP server process");
PROCESS(alarm_messages, "Alarm messages process");
AUTOSTART_PROCESSES(&p3_udp_server_process, &alarm_messages);

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(p3_udp_server_process, ev, data)
{

  PROCESS_BEGIN();

  printf("Central server process started\n");

  NETSTACK_ROUTING.root_start();

  //Connection to the different devices
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);
  simple_udp_register(&room2_conn, UDP_SERVER_PORT, NULL, UDP_ROOM_2_PORT, udp_rx_callback);
  simple_udp_register(&acc_conn, UDP_SERVER_PORT, NULL, UDP_PATIENT_ACC, udp_rx_callback);
  simple_udp_register(&hrt_snsr_conn, UDP_SERVER_PORT, NULL, UDP_PATIENT_HRT, udp_rx_callback);


  PROCESS_END();
}

PROCESS_THREAD(alarm_messages, ev, data){

	//Alarm timers
	static struct etimer alarm_timer;

	PROCESS_BEGIN();

	//Alarm timers set up
	etimer_set(&alarm_timer, CLOCK_SECOND * 1);

	while(1){
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&alarm_timer));
		//Accelerometer value over the threshold
		if(fall_alarm){
			if(fall_alarm_counter < 10){
				leds_toggle(LEDS_RED);
				printf("WARNING: Patient has fallen down!");
				fall_alarm_counter++;
			}
			else{
				fall_alarm_counter = 0;
				fall_alarm = 0;
			}
		}
		//Heartbeat sensor higher than usual (patient in other room)
		if(heartbeat_alarm){ 
            if(hrtbt_alarm_counter < 10){ 
                leds_toggle(LEDS_GREEN); 
                printf("WARNING: Patient is in other room\n"); 
                hrtbt_alarm_counter++; 
            } 
            else{ 
                hrtbt_alarm_counter = 0; 
                heartbeat_alarm = 0; 
            } 
        }
		//Heartbeat abnormally high or low
        if(critical_hrtbt){ 
            if(chrtbt_alarm_counter < 10){ 
                leds_toggle(LEDS_GREEN); 
                printf("WARNING: Patient's heartbeat is critical\n"); 
                chrtbt_alarm_counter++; 
            } 
            else{ 
                chrtbt_alarm_counter = 0; 
                critical_hrtbt = 0; 
            } 
        } 
		//Abnormal temperature or humidity in room 1
        if(room1_alarm){ 
            if(room_1_alarm_counter < 10){ 
                leds_toggle(LEDS_RED); 
                printf("WARNING: Check Room 1 Temperature or Humidity: abnormal value in one of them\n"); 
                room_1_alarm_counter++; 
            } 
            else{ 
                room_1_alarm_counter = 0; 
                room1_alarm = 0; 
            } 
        } 
		etimer_reset(&alarm_timer);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
