
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

#include "home_i2c.h"
#include "home_8574.h"

#define I2C (i2c_port_t) 0
#define SDA      GPIO_NUM_21
#define SCL      GPIO_NUM_22

const char *TAG =  "ONOFF";

Home_i2c bus = Home_i2c();
Home_8574 h8574 = Home_8574(); 

extern "C" void app_main()
{
    //I2C master bus oluşturuluyor
    ESP_LOGI(TAG,"I2C bus oluşturuluyor");
    ESP_ERROR_CHECK(bus.init_bus(SDA,SCL,I2C));
    //bus içine 8574 ilave ediliyor
    ESP_ERROR_CHECK_WITHOUT_ABORT(h8574.init_device(&bus,0x20));
    ESP_LOGI(TAG,"IO Entegresi bulundu");

    while (1) {
        h8574.port_write(0x00);
        ESP_LOGI(TAG,"Out 0x00");
        vTaskDelay(pdMS_TO_TICKS(100));
        h8574.port_write(0xff);
        ESP_LOGI(TAG,"Out 0xFF");
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}