#include "esp_log.h"
#include "driver/ledc.h"
#include "driver/gpio.h"
#include "out_pwm.h"

static const char *TAG = "PWM";
#define MAX_DUTY 4000
#define MIN_DUTY 0
#define FADE_TIME 2000

#define GPIO_PWM_CHECK(a, str, ret_val)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

/*
static bool cb_ledc_fade_end_event(const ledc_cb_param_t *param, void *user_arg)
{
    portBASE_TYPE taskAwoken = pdFALSE;

    if (param->event == LEDC_FADE_END_EVT) {
        SemaphoreHandle_t counting_sem = (SemaphoreHandle_t) user_arg;
        xSemaphoreGiveFromISR(counting_sem, &taskAwoken);
    }

    return (taskAwoken == pdTRUE);
}
*/

esp_err_t out_pwm_init(const out_pwm_config_t *config)
{
    GPIO_PWM_CHECK(NULL != config, "Pointer of config is invalid", ESP_ERR_INVALID_ARG);

    ledc_timer_config_t ledc_timer = {
            .duty_resolution = LEDC_TIMER_13_BIT, // resolution of PWM duty
            .freq_hz = 5000,                      // frequency of PWM signal
            .speed_mode = LEDC_LOW_SPEED_MODE,           // timer mode
            .timer_num = LEDC_TIMER_1,            // timer index
            .clk_cfg = LEDC_AUTO_CLK,              // Auto select the source clock
        };
    ledc_timer_config(&ledc_timer);
    ledc_channel_config_t chn = {
    		.channel    = config->channel,
    		.duty       = 0,
    		.gpio_num   = config->gpio_num,
    		.speed_mode = LEDC_LOW_SPEED_MODE,
    		.hpoint     = 0,
    		.timer_sel  = LEDC_TIMER_1,
    		.flags.output_invert = 0
    };
    ledc_channel_config(&chn);
    ledc_fade_func_install(0);
    /*
    ledc_cbs_t callbacks = {
    		.fade_cb = cb_ledc_fade_end_event
    };
    SemaphoreHandle_t count_sem = xSemaphoreCreateCounting(1,0);
    ledc_cb_register(chn.speed_mode,chn.channel,&callbacks,(void*)count_sem);
  */
    return ESP_OK;
}

esp_err_t out_pwm_deinit(int gpio_num)
{

    return ESP_OK;
}

uint8_t out_pwm_get_level(void *hardware)
{
	out_pwm_config_t *cfg = (out_pwm_config_t *)hardware;
    return cfg->state;
}

uint8_t out_pwm_set_level(void *hardware, uint8_t level)
{
	out_pwm_config_t *cfg = (out_pwm_config_t *)hardware;
	if (cfg->color==0) cfg->color = MAX_DUTY;
	cfg->state = level;
	uint16_t dut = (cfg->state==1) ? cfg->color : MIN_DUTY;

	printf("PWM Set Level %d color %d \n",cfg->state, dut);


	ledc_set_fade_with_time(LEDC_LOW_SPEED_MODE,cfg->channel,dut,FADE_TIME);
    ledc_fade_start(LEDC_LOW_SPEED_MODE,cfg->channel,LEDC_FADE_NO_WAIT);

    return level;
}

uint16_t out_pwm_set_color(void *hardware, uint16_t color)
{
	out_pwm_config_t *cfg = (out_pwm_config_t *)hardware;
	printf("color = %d\n",color);
	cfg->color = color;
	if(cfg->state==1) out_pwm_set_level(hardware,1);
	return cfg->color;
}

uint8_t out_pwm_toggle_level(void *hardware)
{
	out_pwm_config_t *cfg = (out_pwm_config_t *)hardware;
	cfg->state = !cfg->state;
	return out_pwm_set_level(hardware,cfg->state);
}
