/*
 * p3_udp_server_ej2.c
 *
 *  Created on: Nov 12, 2024
 *      Author: user
 */

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
//static struct simple_udp_connection room2_conn;
static struct simple_udp_connection acc_conn;
//static struct simple_udp_connection hrt_snsr_conn;

//Measurement variables
float acceleration = 0;
int accel = 0;

//Alarm variables
uint8_t fall_alarm = 0;

//Alarm counters
int fall_alarm_counter = 0;

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

	else if(strncmp((char*)data, "R1", 2) == 0){//Room 1

		printf("Temperature and Humidity from room 1: %s\n", (char*)data);
	}

	else if(strncmp((char*)data, "R2", 2) == 0){//Room 2
		printf("Temperature and Humidity from room 2%s\n", (char*)data);
	}

	else if(strncmp((char*)data, "Hs", 2) == 0){//Heartbeat sensor

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
  //simple_udp_register(&room2_conn, UDP_SERVER_PORT, NULL, UDP_ROOM_2_PORT, udp_rx_callback);
  simple_udp_register(&acc_conn, UDP_SERVER_PORT, NULL, UDP_PATIENT_ACC, udp_rx_callback);
  //simple_udp_register(&hrt_snsr_conn, UDP_SERVER_PORT, NULL, UDP_PATIENT_HRT, udp_rx_callback);


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
		etimer_reset(&alarm_timer);
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
