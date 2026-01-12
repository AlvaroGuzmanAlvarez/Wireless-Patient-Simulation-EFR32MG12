/*Accelerometer client code*/

#include "contiki.h"

#include <stdio.h> /* For printf() */
#include <math.h>
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

/*---------------------------------------------------------------------------*/

#define UDP_CLIENT_PORT 8767
#define UDP_SERVER_PORT 5678

#define MY_MESSAGE "Test UDP Client"

/*---------------------------------------------------------------------------*/

static struct simple_udp_connection udp_conn;

/*---------------------------------------------------------------------------*/

void udp_rx_callback(struct simple_udp_connection *c,
                                     const uip_ipaddr_t *source_addr,
                                     uint16_t source_port,
                                     const uip_ipaddr_t *dest_addr,
                                     uint16_t dest_port,
                                     const uint8_t *data, uint16_t datalen){
	printf("Received Packet from: ");
	uiplib_ipaddr_print(source_addr);
	printf(", Data; %s\r", (char*)data);

}

/*---------------------------------------------------------------------------*/
PROCESS(p3_udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&p3_udp_client_process);


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(p3_udp_client_process, ev, data)
{
  static struct etimer timer;
  uip_ipaddr_t dest_addr;

  //Sensor variables
  static float accelflo[3] = {0,0,0};
  static  int32_t accelint[3] = {0,0,0};
  static float accelmod = 0.0;
  static int32_t accelmodule = 0;

  PROCESS_BEGIN();


  leds_on(LEDS_RED); 
  printf("Accelerometer process started\n");
  /* Setup a periodic timer (etimer) using etimermr_set, etimer_expired and etimer_reset */

  //Sensor configuration
  GPIO_PinModeSet(gpioPortF, 11, gpioModePushPull, 1);		// inertial, set as out pin (PushPull)
  GPIO_PinOutSet(gpioPortF, 11);							// inertial, enabled

  //Accelerometer start
  IMU_init();
  IMU_config(20);

  //Timer and communication configuration
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
  etimer_set(&timer, CLOCK_SECOND * 0.5);
  leds_toggle(LEDS_RED);

  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

	IMU_getAccelerometerData(accelflo);			// results in mg (g ·10^-3)
	for (int i = 0; i < 3; i++){

		accelint[i] = (int32_t) 1000*accelflo[i];

	}
	//Acceleration module calculation for sending it to the sever
	accelmod = sqrt(accelint[0]*accelint[0] + accelint[1]*accelint[1] + accelint[2]*accelint[2]);
	accelmodule = (int32_t) 1000*accelmod;

	printf("%ld.%ld (%%)\n", accelmodule/1000, accelmodule%1000);

	//Creation of the message to send to the server
	 char message[200];
	 sprintf(message, "AC %ld\n", accelmodule);

	//Sendign message to the central server
	if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_addr) ){

		printf("Sending message to the server\n");
	    simple_udp_sendto(&udp_conn, message, sizeof(message), &dest_addr);
	}
	else{

	    printf("Destination node not reachable yet\n");
	}
	etimer_reset(&timer);
	  
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
