
#include <stdio.h>
#include "sdkconfig.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_event.h"
#include "driver/i2c.h"

#include "master_i2c.h"
#include "dev_8574.h"
#include "jsontool.h"
#include "rs485.h"
#include "storage.h"
#include "classes.h"

#define I2C (i2c_port_t) 0
#define SDA      GPIO_NUM_21
#define SCL      GPIO_NUM_22
#define ROLE1       0
#define ROLE2       1
#define TUS1        2
#define TUS2        3
#define CPC         4

#define TX       GPIO_NUM_26
#define RX       GPIO_NUM_25
#define DIR      GPIO_NUM_13

#define LED      GPIO_NUM_0

#define TUS3     GPIO_NUM_9



const char *TAG =  "ONOFF";

ESP_EVENT_DEFINE_BASE(RS485_DATA_EVENTS);
ESP_EVENT_DEFINE_BASE(MESSAGE_EVENTS);

#define GLOBAL_FILE "/config/global.bin"

Master_i2c bus = Master_i2c();
Dev_8574 pcf[4] = {};
Dev_8574 pcf0 =  Dev_8574();
Storage disk = Storage();
RS485_config_t rs485_cfg={};
RS485 rs485 = RS485();
home_global_config_t GlobalConfig = {};

SemaphoreHandle_t register_ready;
SemaphoreHandle_t status_ready;
SemaphoreHandle_t IACK_ready;
SemaphoreHandle_t REGOK_ready;

SemaphoreHandle_t mainbox_ready;
SemaphoreHandle_t mainbox_ok;
bool SERVER_READY = false;

Base_Function *function_find(uint8_t id);
Base_Function *find_register(uint8_t sender, uint8_t rid);


void global_default_config(void)
{
     GlobalConfig.home_default = 1; 
     strcpy((char*)GlobalConfig.device_name, "Room_CPU2");
     strcpy((char*)GlobalConfig.mqtt_server,"");
     strcpy((char*)GlobalConfig.license, "");
     GlobalConfig.mqtt_keepalive = 0; 
     GlobalConfig.start_value = 0;
     GlobalConfig.device_id = (1<<5)| 1;
     GlobalConfig.http_start = 1;
     //GlobalConfig.tcp_start = 1;
     GlobalConfig.reset_servisi = 0;
     disk.file_control(GLOBAL_FILE);
     disk.write_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
}

#include "tool/events.cpp"
#include "tool/functions.cpp"
#include "tool/global.cpp"
#include "tool/other.cpp"


extern "C" void app_main()
{

    if(esp_event_loop_create_default()!=ESP_OK) {ESP_LOGE(TAG,"esp_event_loop_create_default ERROR "); }

    gpio_config_t io_conf = {};
	io_conf.mode = GPIO_MODE_INPUT_OUTPUT;
	io_conf.pin_bit_mask = (1ULL<<LED);
	io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&io_conf);

    gpio_config_t io_conf1 = {};
	io_conf1.mode = GPIO_MODE_INPUT;
	io_conf1.pin_bit_mask = (1ULL<<TUS3);
	io_conf1.pull_down_en = GPIO_PULLDOWN_DISABLE;
	io_conf1.pull_up_en = GPIO_PULLUP_ENABLE;
	gpio_config(&io_conf1);

    gpio_set_level(LED,0);

    //I2C master bus oluşturuluyor
    ESP_LOGI(TAG,"I2C bus oluşturuluyor");
    ESP_ERROR_CHECK(bus.init_bus(SDA,SCL,I2C));
    //bus içine 8574 ilave ediliyor
    ESP_ERROR_CHECK_WITHOUT_ABORT(pcf0.init_device(&bus,0x20));
    ESP_LOGI(TAG,"IO Entegresi bulundu");
    pcf[0] = pcf0;

    ESP_LOGI(TAG,"NVS Flash Init");
    esp_err_t ret = nvs_flash_init();
        if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
            ESP_ERROR_CHECK( nvs_flash_erase() );
            ret = nvs_flash_init();
        }
     ESP_ERROR_CHECK( ret );
    
    ESP_LOGI(TAG,"FFS Init");
    ret = disk.init();
    ESP_ERROR_CHECK (ret);
    
    disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig), 0);
    if (GlobalConfig.home_default==0 ) {
        //Global ayarlar diskte kayıtlı değil. Kaydet.
         global_default_config();
         disk.read_file(GLOBAL_FILE,&GlobalConfig,sizeof(GlobalConfig),0);
         if (GlobalConfig.home_default==0 ) printf( "\n\nGlobal Initilalize File ERROR !...\n\n");
    }
    disk.list("/config", "*.*");

   // rs485_input_test(LED);

    rs485_cfg.uart_num = 1;
    rs485_cfg.dev_num  = GlobalConfig.device_id;
    rs485_cfg.rx_pin   = RX;
    rs485_cfg.tx_pin   = TX;
    rs485_cfg.oe_pin   = DIR;
    rs485_cfg.baud     = 115200;
    rs485.initialize(&rs485_cfg, (gpio_num_t)-1);

    Read_functions(disk,pcf);   
    function_list(true);

    ESP_ERROR_CHECK(esp_event_handler_instance_register(RS485_DATA_EVENTS, ESP_EVENT_ANY_ID, rs485_handler, NULL, NULL)); 
    ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_OUT_EVENTS, ESP_EVENT_ANY_ID, function_out_handler, NULL, NULL)); 


    ESP_LOGW(TAG,"%d Ana Cihaz Kontrol Edilecek..",GlobalConfig.device_id );
    mainbox_ready = xSemaphoreCreateBinary();
    assert(mainbox_ready);
    mainbox_ok = xSemaphoreCreateBinary();
    assert(mainbox_ok);
    xTaskCreate(mbready_task, "mb_01", 2048, NULL, 10, NULL);
    xSemaphoreTake(mainbox_ready, portMAX_DELAY);
    vSemaphoreDelete(mainbox_ready);
    vSemaphoreDelete(mainbox_ok);
    mainbox_ok=NULL;
    mainbox_ready=NULL;

    if (SERVER_READY) ESP_LOGI(TAG,"Ana Cihaz BULUNDU.." ); else ESP_LOGE(TAG,"Ana Cihaz CEVAP VERMIYOR.." );

    
    ESP_LOGW(TAG,"Fonksiyon Kayıtları Kontrol Ediliyor.." );

    //Fonksiyonların önceki register durumları okunuyor ve cihazlar register ediliyor
    uint8_t rre = 0;
    for (uint8_t i=0;i<MAX_DEVICE;i++)
    {
      function_reg_t fun = {};
      disk.read_file(FUNCTION_FILE,&fun,sizeof(fun),i);
      if (fun.device_id>0) 
      {
         Base_Function *a = function_find(fun.device_id);
            if (a!=NULL) {
              a->genel.register_id = fun.register_id;
              a->genel.register_device_id = fun.register_device_id;
              a->genel.registered = true;
              rre++;
            }
      }
    }
    ESP_LOGW(TAG,"%d local function yeniden register edildi.",rre); 
    uint8_t nreg = function_list(false);
    if (nreg>0) {
        ESP_LOGE(TAG,"Register olmamış %d fonksiyon bulundu. Register edilecek",nreg); 
        //function_register_all(NULL);
    }

    ESP_LOGI(TAG,"STARTED.....");

    while (1) {

        if (gpio_get_level(TUS3)==0)
        {
           ESP_LOGW(TAG,"Register BASLIYOR");
           vTaskDelay(pdMS_TO_TICKS(3000));
           xTaskCreate(function_register_all, "mb_00", 2048, NULL, 10, NULL); 
           ESP_LOGW(TAG,"Register BASLATILDI");
           vTaskDelay(pdMS_TO_TICKS(3000));

        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}