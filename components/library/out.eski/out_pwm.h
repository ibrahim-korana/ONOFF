
#ifndef _IOT_OUT_PWM_H_
#define _IOT_OUT_PWM_H_

#include "driver/ledc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int32_t gpio_num;
    uint8_t channel;
    uint8_t state;
    uint16_t color;
} out_pwm_config_t;

esp_err_t out_pwm_init(const out_pwm_config_t *config);
esp_err_t out_pwm_deinit(int gpio_num);
uint8_t out_pwm_get_level(void *hardware);
uint8_t out_pwm_set_level(void *hardware, uint8_t level);
uint8_t out_pwm_toggle_level(void *hardware);
uint16_t out_pwm_set_color(void *hardware, uint16_t color);


#ifdef __cplusplus
}
#endif

#endif
