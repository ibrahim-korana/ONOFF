
#include "button_8574.h"
#include "../pcf/ice_pcf8574.h"

static const char *TAG = "8574 button";

#define GPIO_BTN_CHECK(a, str, ret_val)                          \
    if (!(a))                                                     \
    {                                                             \
        ESP_LOGE(TAG, "%s(%d): %s", __FUNCTION__, __LINE__, str); \
        return (ret_val);                                         \
    }

esp_err_t button_8574_init(const button_8574_config_t *config)
{
    GPIO_BTN_CHECK(NULL != config, "Pointer of config is invalid", ESP_ERR_INVALID_ARG);
    
    return ESP_OK;
}

esp_err_t button_8574_deinit(int gpio_num)
{
    return ESP_OK;
}

uint8_t button_8574_get_key_level(void *hardware)
{
    button_8574_config_t *cfg = (button_8574_config_t *) hardware;
    uint8_t dt=0;
    //pcf8574_port_read(cfg->device,&dt);
    //i2c_dev_read(cfg->device, NULL, 0, &dt, 1);
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
        i2c_master_write_byte(cmd, (cfg->device->addr << 1) | 1, true);
        i2c_master_read(cmd, &dt, 1, I2C_MASTER_LAST_NACK);
        i2c_master_stop(cmd);

        esp_err_t res = i2c_master_cmd_begin(cfg->device->port, cmd, pdMS_TO_TICKS(5000));
        if (res != ESP_OK) printf("HATA\n");
            //ESP_LOGE(TAG, "Could not read from device [0x%02x at %d]: %d (%s)", dev->addr, dev->port, res, esp_err_to_name(res));

        i2c_cmd_link_delete(cmd);


    if (cfg->pin_num==6) 
      printf("button read %d %02X\n",dt,cfg->device->addr);

    return (dt & (1 << (uint8_t)cfg->pin_num)) > 0;
}


