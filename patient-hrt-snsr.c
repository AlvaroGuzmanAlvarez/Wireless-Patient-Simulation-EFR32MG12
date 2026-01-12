/*Heart sensor client code*/

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
#include "em_adc.h" //For ADC

/*---------------------------------------------------------------------------*/

#define UDP_CLIENT_PORT 8766
#define UDP_SERVER_PORT 5678

#define MY_MESSAGE "Test UDP Client"

//ADC frequency adjustment to 16MHz
#define adcFreq 16000000

/*---------------------------------------------------------------------------*/

static struct simple_udp_connection udp_conn;

/*------------------------ADC variables--------------------------------------*/

ADC_Init_TypeDef init = ADC_INIT_DEFAULT; 
ADC_InitSingle_TypeDef initSingle = ADC_INITSINGLE_DEFAULT;

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

void initADC() { 
 
  //Initialize clock (we use ADC0) 
  CMU_ClockEnable(cmuClock_ADC0, true); 
 
  //Modify init structs and initialize 
  init.prescale = ADC_PrescaleCalc(adcFreq,0); 
 
  initSingle.diff = false; //single ended 
  initSingle.reference = adcRef2V5; //Internal 2.5V reference 
 
  initSingle.resolution = adcRes12Bit; //12-bit resolution (Bit resoluton of the uP) 
  initSingle.acqTime = adcAcqTime4; //Acquisition time set 
 
  initSingle.posSel = adcPosSelAPORT1YCH1; //Channel 1 -> PC1 pin 
 
  ADC_Init(ADC0, &init); 
  ADC_InitSingle(ADC0, &initSingle); 
 
} 

/*---------------------------------------------------------------------------*/
PROCESS(p3_udp_client_process, "UDP client process");
AUTOSTART_PROCESSES(&p3_udp_client_process);


/*---------------------------------------------------------------------------*/
PROCESS_THREAD(p3_udp_client_process, ev, data)
{
  static struct etimer timer;
  uip_ipaddr_t dest_addr;

  PROCESS_BEGIN();

  leds_on(LEDS_RED); 
  printf("Patient heartbeat monitoring started\n");

  //ADC initialization
  initADC();

  //Timer and communication configuration
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
  etimer_set(&timer, CLOCK_SECOND * 0.5);

  while(1) {

  /* Wait for the periodic timer to expire and then restart the timer. */ 
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timer)); 
 
    //ADC start 
    ADC_Start(ADC0, adcStartSingle); 
 
    //Wait for the conversion to end 
    while(!(ADC0->STATUS & _ADC_STATUS_SINGLEDV_MASK)); 

    //Get ADC result
    sample = ADC_DataSingleGet(ADC0);
    //printf("raw sample: %ld \n", sample); 

    //Conversion to BPM 
    BPM = sample*78/2420; //Experimental measurement 
    printf("BPM: %ld \n", BPM);

    //Message to be sent
    char message[200]; 
    sprintf(message, "HS %ld\n", BPM); 
 
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_addr) ){ 
      printf("Sending heartbeat to the server\n"); 
      simple_udp_sendto(&udp_conn, message, sizeof(message), &dest_addr); 
    }else{ 
      printf("Destination node not reachable yet\n"); 
    } 
 
    etimer_reset(&timer); 
 
  } 

  PROCESS_END();

}
/*---------------------------------------------------------------------------*/
