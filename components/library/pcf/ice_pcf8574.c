

#include <esp_err.h>
#include "esp_idf_lib_helpers.h"
#include "i2cdev.h"
#include "ice_pcf8574.h"

#define I2C_FREQ_HZ 10000
#define MY_I2C_FREQ_HZ 400000


#define CHECK(x) do { esp_err_t __; if ((__ = x) != ESP_OK) return __; } while (0)
#define CHECK_ARG(VAL) do { if (!(VAL)) return ESP_ERR_INVALID_ARG; } while (0)
#define BV(x) (1 << (x))

static esp_err_t read_port(i2c_dev_t *dev, uint8_t *val)
{
    CHECK_ARG(dev && val);

    //I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev,i2c_dev_read(dev, NULL, 0, val, 1));
    //I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

static esp_err_t write_port(i2c_dev_t *dev, uint8_t val)
{
    CHECK_ARG(dev);

    //I2C_DEV_TAKE_MUTEX(dev);
    I2C_DEV_CHECK(dev, i2c_dev_write(dev, NULL, 0, &val, 1));
    //I2C_DEV_GIVE_MUTEX(dev);

    return ESP_OK;
}

///////////////////////////////////////////////////////////////////////////////

esp_err_t pcf8574_init_desc(i2c_dev_t *dev, uint8_t addr, i2c_port_t port, gpio_num_t sda_gpio, gpio_num_t scl_gpio)
{
    CHECK_ARG(dev);
    //CHECK_ARG(addr & 0x20);

    dev->port = port;
    dev->addr = addr;
    dev->cfg.sda_io_num = sda_gpio;
    dev->cfg.scl_io_num = scl_gpio;
    dev->cfg.sda_pullup_en = true;
    dev->cfg.scl_pullup_en = true;
    dev->cfg.master.clk_speed = MY_I2C_FREQ_HZ;


    return i2c_dev_create_mutex(dev);
}

esp_err_t pcf8574_free_desc(i2c_dev_t *dev)
{
    CHECK_ARG(dev);

    return i2c_dev_delete_mutex(dev);
}

esp_err_t pcf8574_port_read(i2c_dev_t *dev, uint8_t *val)
{
    return read_port(dev, val);
}

esp_err_t pcf8574_port_write(i2c_dev_t *dev, uint8_t val)
{
    return write_port(dev, val);
}

esp_err_t pcf8574_pin_write(i2c_dev_t *dev, uint8_t pin, uint8_t val)
{
    uint8_t dt=0;
    ESP_ERROR_CHECK(pcf8574_port_read(dev,&dt));
    if (val == 0)
    {
        dt &= ~(1 << pin);
    } else {
        dt |= (1 << pin);
    }
    return write_port(dev, dt);
}

uint8_t pcf8574_pin_read(i2c_dev_t *dev, uint8_t pin)
{
    uint8_t dt=0;
    pcf8574_port_read(dev,&dt);
    return (dt & (1 << (uint8_t)pin)) > 0;
}

esp_err_t pcf8574_probe(i2c_dev_t *pp, uint8_t addr,bool kontrol)
{
    esp_err_t err = ESP_OK;

   err = pcf8574_init_desc(pp, addr, (i2c_port_t)0, (gpio_num_t)21, (gpio_num_t)22);

   
   int i=0,j=0;
   /*
   err = i2c_dev_probe(pp,I2C_DEV_READ);
   while(err!=ESP_OK)
         {
            vTaskDelay(1/portTICK_PERIOD_MS);
            //err = pcf8574_init_desc(pp, addr, (i2c_port_t)0, (gpio_num_t)21, (gpio_num_t)22);
            //i2c_set_timeout((i2c_port_t)0,500000);
            if (i++>1000) return ESP_FAIL;
            err = i2c_dev_probe(pp,I2C_DEV_READ);
            printf("%02X FAIL %X %d\n",addr, err, i);
         }
         */

   err = i2c_dev_read(pp,NULL,0,&i,1);
   i2c_filter_enable(pp->port,4);
   printf("%02X READ %X %02X\n",addr, err, i);
   if (kontrol) if (i==0x00) err=ESP_FAIL;
   while(err!=ESP_OK)
         {
            vTaskDelay(1/portTICK_PERIOD_MS);
            //err = pcf8574_init_desc(pp, addr, (i2c_port_t)0, (gpio_num_t)21, (gpio_num_t)22);
            //i2c_set_timeout((i2c_port_t)0,500000);
            if (j++>1000) return ESP_FAIL;
            err = i2c_dev_read(pp,NULL,0,&i,1);
            if (kontrol) if (i==0x00) err=ESP_FAIL;
            printf("%02X FAIL %X %d %02X\n",addr, err,j,i);
         }     

 
  return err;
}