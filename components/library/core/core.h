#ifndef _OTEL_CORE_H
#define _OTEL_CORE_H

#include <stdio.h>
#include "esp_event.h"


#define MAX_DEVICE 50
#define MAX_FUNCTION 50

#define FIRE_DELAY 5

typedef enum {
    HOME_WIFI_DEFAULT = 0,
    HOME_WIFI_AP,     //Access point oluşturur
    HOME_WIFI_STA,    //Access station oluşturur
    HOME_WIFI_AUTO,   //Access station oluşturur. AP bağlantı hatası oluşursa AP oluşturur 
    HOME_ETHERNET,    //SPI ethernet
    HOME_NETWORK_DISABLE,//Wifi kapatır   
} home_wifi_type_t;

typedef enum {
    IP_DEFAULT = 0,
    STATIC_IP, 
    DYNAMIC_IP, 
} home_ipstat_type_t;

typedef enum {
    WAN_DEFAULT = 0,
    WAN_ETHERNET,
    WAN_WIFI,
} home_wan_type_t;

typedef struct {
    uint8_t home_default;
    home_wifi_type_t wifi_type;
    home_wan_type_t wan_type;
    uint8_t wifi_ssid[32];        //AP Station de kullanılacak SSID
    uint8_t wifi_pass[32];        //AP Station de kullanılacak Şifre 
    home_ipstat_type_t ipstat;
    uint8_t ip[17];               //Static ip 
    uint8_t netmask[17];          //Static netmask
    uint8_t gateway[17];          //Static gateway
    uint32_t home_ip;
    uint32_t home_netmask;
    uint32_t home_gateway;
    uint32_t home_broadcast;
    uint8_t mac[6];
    uint8_t channel;
    uint8_t dns[17]; 
    uint8_t backup_dns[17]; 
} home_network_config_t;

typedef struct {
  union {
	struct {
		uint16_t altodano:2;
		uint16_t binano:5;
		uint16_t katno:4;
		uint16_t odano:5;
	       } _room;
	    uint16_t room;
  };
} odano_t;

typedef struct {
    uint8_t home_default;
    uint8_t device_name[32];
    uint8_t mqtt_server[32];
    uint8_t mqtt_keepalive;
    uint8_t start_value;
    odano_t odano;
    uint8_t device_id;
    uint8_t comminication;
    uint8_t http_start;
    uint8_t tcpserver_start;
    uint8_t license[16];
    uint8_t random_mac;
    uint8_t rawmac[4];
    uint8_t reset_servisi;
} home_global_config_t;

typedef struct prg_location {
    char name[20];
    uint8_t page;
    void *next;
} prg_location_t;

typedef enum {
  TR_NONE = 0,
  TR_UDP = 1,
  TR_SERIAL,
  TR_WIRE,
  TR_LOCAL,
  TR_ESPNOW,
  TR_TCP
} transmisyon_t;


typedef enum {
	RET_OK = 0,
	  RET_NO_ACK,
	  RET_TIMEOUT,
	  RET_COLLESION,
	  RET_NO_CLIENT,
	  RET_HARDWARE_ERROR,
	  RET_BUSY,
	  RET_RESPONSE,
    RET_NONE,
} return_type_t;

typedef enum {
    PORT_NC = 0,
    PORT_INPORT,    //in
    PORT_INPULS,    //in
    PORT_OUTPORT,   //out
    PORT_SENSOR,    //in
    PORT_VIRTUAL,   //in
    PORT_FIRE,      //in
    PORT_WATER,     //in
    PORT_GAS,       //in
    PORT_EMERGENCY, //in
	  PORT_PWM,
} port_type_t;



/*Rs485 tanımları*/


typedef enum {
  PAKET_NORMAL = 0,
  PAKET_PING,
  PAKET_PONG,
  PAKET_ACK,
  PAKET_RESPONSE,
  PAKET_START,
  PAKET_FIN,
} RS485_paket_type_t;

typedef struct {
  RS485_paket_type_t paket_type:4;
  uint8_t paket_ack:1;
  uint8_t frame_ack:1;
} RS485_flag_t;

typedef struct {
  uint8_t sender;
  uint8_t receiver;
  RS485_flag_t flag;
  uint8_t total_pk;
  uint8_t current_pk;
  uint8_t id;
  uint8_t data_len;
} RS485_header_t;

typedef struct {
  uint8_t uart_num;
  uint8_t dev_num;
  uint8_t rx_pin;
  uint8_t tx_pin;
  uint8_t oe_pin;
  int baud;
} RS485_config_t;

typedef enum {
  SENDER = 0,
  RECEIVER,
} rs_mode_t;

typedef struct
{
    uint8_t *data;
    uint8_t receiver;
    RS485_paket_type_t paket_type;
    void *self;
} Data_t;

typedef struct
{
    RS485_header_t *header;
    uint8_t *data;
} Paket_t;


typedef struct device_register {
    uint8_t device_id;
    uint32_t ip;
    transmisyon_t transmisyon;
    uint8_t active;
    uint8_t oldactive;
    uint8_t function_count;
    char mac[14];
    int socket;
    uint8_t mac0[6];
    void *next;
} device_register_t;

struct function_reg_t {
    char name[20];
    char auname[20];
    uint8_t device_id;
    uint8_t register_id;
    uint8_t register_device_id;
    uint8_t prg_loc;
    transmisyon_t transmisyon;
};

typedef struct {
	uint8_t state;
} led_events_data_t;

typedef struct {
     char *data;
     int data_len;
     int socket;
} tcp_events_data_t;

typedef struct {
     char *data;
     int data_len;
     uint8_t sender;
} rs485_events_data_t;

typedef struct {
     char *topic;
     int topic_len;
     char *data;
     int data_len;
} mqtt_events_data_t;

typedef struct {
  uint8_t sender;
  char *data;
  transmisyon_t trn;
} response_par_t;

/*
      Fonksiyon Yardımcıları
 */

struct function_prop_t {
    char name[20];
    char uname[20];
    uint8_t device_id;
    uint8_t register_id;
    uint8_t register_device_id;
    bool active;
    bool virtual_device;
    bool registered;
    transmisyon_t location;
    uint8_t oid;
    uint8_t icon;
};

typedef enum {
    ACTION_INPUT = 0,
    ACTION_OUTPUT,
} port_action_type_t;

struct port_action_s
{   port_action_type_t action_type;
    void *port;
} ;
typedef struct port_action_s port_action_t;

typedef struct {
    bool stat;
    bool active;
    uint8_t status;
    float temp;
    float set_temp;
    uint64_t color;
    uint8_t ircom[16];
    uint8_t irval[16];
    uint8_t counter;
    bool first;
    uint8_t id;
    uint8_t oid;
} home_status_t;

typedef struct {
  uint8_t name[16];
  uint8_t stat;
  float temp;
  float set_temp;
  uint8_t temp_mode;
  uint8_t active;
  uint8_t sender;
  uint16_t dnd_status;
} home_virtual_t;

typedef struct {
  char *txt;
  uint8_t id;
} home_message_t;

/*
      Event Tanımları
*/

ESP_EVENT_DECLARE_BASE(LED_EVENTS);
ESP_EVENT_DECLARE_BASE(ROOM_EVENTS);
ESP_EVENT_DECLARE_BASE(TCP_SERVER_EVENTS);
ESP_EVENT_DECLARE_BASE(SYSTEM_EVENTS);
ESP_EVENT_DECLARE_BASE(MQTT_LOCAL_EVENTS);
ESP_EVENT_DECLARE_BASE(RS485_DATA_EVENTS);

//Herhangi bir fonk aksiyon ürettiginde kullanılır
//parametresi Fonksiyonun kendisidir. 
ESP_EVENT_DECLARE_BASE(FUNCTION_OUT_EVENTS);

//Fonksiyonlara status göndermek için kullanılır
//Tüm fonksiyonlar bunu dinler
//Paramertesi status tur 
ESP_EVENT_DECLARE_BASE(FUNCTION_IN_EVENTS);

//fonksiyon remote ise bu mesaj oluşut
ESP_EVENT_DECLARE_BASE(FUNCTION_REMOTE_EVENTS);

//Virtual portlara ait aksiyonlarda bu event kullanılır
ESP_EVENT_DECLARE_BASE(VIRTUAL_EVENTS);

//Alarm emirleri
ESP_EVENT_DECLARE_BASE(ALARM_EVENTS);

//Virtual fonksiyon emirleri
ESP_EVENT_DECLARE_BASE(VIRTUAL_COMMAND_EVENTS);

//Virtual Sensore gidecek eventler
ESP_EVENT_DECLARE_BASE(VIRTUAL_SEND_EVENTS);

//Home events
ESP_EVENT_DECLARE_BASE(HOME_EVENTS);
ESP_EVENT_DECLARE_BASE(MESSAGE_EVENTS);

enum {
	LED_EVENTS_ETH,
	LED_EVENTS_WIFI,
	LED_EVENTS_MQTT,
  TCP_EVENTS_DATA,
  MQTT_EVENTS_DATA,
  SYSTEM_RESET,
  SYSTEM_DEFAULT_RESET,
  RS485_EVENTS_BROADCAST,
  RS485_EVENTS_DATA,
  CALLBACK_DATA,
  ROOM_EXT_ON,
  ROOM_EXT_OFF,
  ROOM_FON,
  ROOM_ACTION,
  ROOM_DISABLE,
  FUNCTION_ACTION,
  VIRTUAL_DATA,
  VIRTUAL_SEND_DATA,
  ALARM_DATA,
  DND_ON,
  DND_OFF,
  DOOR_OPEN_COMMAND,
  DOOR_CLOSE_COMMAND,
  EMERGENCY_ON,
  EMERGENCY_OFF,
  CLEANOK_ON,
  CLEAN_ON,
  CLEAN_OFF,
  CHECK_IN,
  CHECK_OUT,
  MOVEMEND,
  FIRE_ON,
  FIRE_OFF,
  MESSAGE,
  MESSAGE_IN,
  MESSAGE_SYSTEM,
  MESSAGE_CLNOK,
  MESSAGE_EMERGENCY,
  MESSAGE_FIRE,
  MESSAGE_ALARM,
  LAMP_ALL_ON,
  LAMP_ALL_OFF,
  DAYCLEAN_ON,
  DAYCLEAN_OFF,
};

typedef enum {
	ROOM_ON,
	ROOM_OFF,
	MAIN_DOOR_ON,
	MAIN_DOOR_OFF,
} room_event_t;


/*
         UTIL FUNCTIONS
*/
port_type_t port_type_convert(char * aa);
void port_type_string(port_type_t tp,char *aa);
void transmisyon_string(transmisyon_t tt,char *aa);


/*
         CALLBACK
*/

typedef void (*default_callback_t)(void *arg);
typedef void (*web_callback_t)(home_network_config_t net, home_global_config_t glob);
typedef void (*rs485_callback_t)(char *data, uint8_t sender, transmisyon_t transmisyon);

typedef void (*port_callback_t)(void *arg, port_action_t action);
typedef void (*register_callback_t)(void *arg);
typedef void (*function_callback_t)(void *prt, home_status_t stat);
typedef void (*change_callback_t)(void *arg);


class OdaNo {
	public:
	  	  OdaNo(odano_t odn) {
	  		  _odano = odn;
	  		  mem = (char *)malloc(10);
	  	  }
	  	  ~OdaNo() {free(mem);};
	  	  uint8_t get_kat() {return _odano._room.katno;}
	  	  uint8_t get_bina() {return _odano._room.binano;}
	  	  uint8_t get_oda() {return _odano._room.odano;}
	  	  uint8_t get_connection() {return _odano._room.altodano;}
	  	  char *get_oda_string() {
	  		 if (_odano._room.altodano==0)
	  		 {
	  			 sprintf(mem,"%1d%1d%02d",_odano._room.binano,_odano._room.katno,_odano._room.odano);
	  		 } else {
	  			sprintf(mem,"%1d%1d%02d.%1d",_odano._room.binano,_odano._room.katno,_odano._room.odano,_odano._room.altodano);
	  		 }
	  		 return mem;
	  	                         }
        void set_oda(uint16_t odn)
          {
            _odano.room = odn;
          }    
        uint16_t get_oda_id(void)
        {
          return _odano.room;
        }                      
	private:
	  	  odano_t _odano;
	  	  char *mem;

};


#endif
