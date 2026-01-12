/*Room 2 client code*/

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

/*---------------------------------------------------------------------------*/

#define UDP_CLIENT_PORT 8766
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
  uint32_t hume = 0;
  int32_t temper = 0;

  PROCESS_BEGIN();


  printf("Room 1 monitoring started\n");
/* Setup a periodic timer (etimer) using etimermr_set, etimer_expired and etimer_reset */

  //Sensor configuration
  GPIO_PinModeSet(gpioPortF, 8, gpioModePushPull, 1);		// consum, set as out pin (PushPull)
  GPIO_PinModeSet(gpioPortF, 9, gpioModePushPull, 1);		// T/H, set as out pin (PushPull)
  GPIO_PinOutSet(gpioPortF, 9);								// T/H, enabled

  I2CSPM_Init_TypeDef i2cspmInit = I2CSPM_INIT_DEFAULT; 	// I2C configuration
  I2CSPM_Init(&i2cspmInit);									// I2C configuration
  SI7021_init();											// TEMP/HUM Sensor confi



  //Timer and communication configuration
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
  etimer_set(&timer, CLOCK_SECOND * 2);

  while(1) {

    /* Wait for the periodic timer to expire and then restart the timer. */
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer));

    //Sensor measurement
    SI7021_measure(&hume, &temper);

	char message1[10]; 
    char message2[10]; 
    sprintf(message1, "R2H %lu\n", hume); 
    sprintf(message2, "R2T %lu\n", temper);

    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_addr) ){
    	printf("Sending messages to the server\n");
		printf("Humidity: %lu.%lu (%%) ; Temperature: %lu.%lu (ºC)\n", hume/1000, hume%1000, temper/1000, temper%1000);
    	simple_udp_sendto(&udp_conn, message1, sizeof(message1), &dest_addr); 
      	simple_udp_sendto(&udp_conn, message2, sizeof(message2), &dest_addr);
    }else{
    	printf("Destination node not reachable yet\n");
    }

    etimer_reset(&timer);

  }
  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
