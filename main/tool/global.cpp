
return_type_t Global_Send(char *data,uint8_t sender, transmisyon_t transmisyon)
{
 // if (transmisyon==TR_UDP) tcpserver.Send(data,sender);
  return_type_t ret = RET_NONE;
  if (transmisyon==TR_SERIAL) {
        while(rs485.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
        ret = rs485.Sender(data,sender);
                              }
  return ret;
}



#include "rs485.h"

void rs485_output_test(void)
{
bool rep = true;
uint16_t counter = 0;

RS485_config_t rs485_cfg={};
rs485_cfg.uart_num = 1;
rs485_cfg.dev_num  = 253;
rs485_cfg.rx_pin   = 25;
rs485_cfg.tx_pin   = 26;
rs485_cfg.oe_pin   = 13;
rs485_cfg.baud     = 460800;

uart_driver_delete((uart_port_t)rs485_cfg.uart_num);

        uart_config_t uart_config = {};
        uart_config.baud_rate = 115200;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.rx_flow_ctrl_thresh = 122;

ESP_ERROR_CHECK(uart_driver_install((uart_port_t)rs485_cfg.uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
ESP_ERROR_CHECK(uart_param_config((uart_port_t)rs485_cfg.uart_num, &uart_config));
ESP_ERROR_CHECK(uart_set_pin((uart_port_t)rs485_cfg.uart_num, rs485_cfg.tx_pin, rs485_cfg.rx_pin, rs485_cfg.oe_pin, UART_PIN_NO_CHANGE));
ESP_ERROR_CHECK(uart_set_mode((uart_port_t)rs485_cfg.uart_num, UART_MODE_RS485_HALF_DUPLEX));
ESP_ERROR_CHECK(uart_set_rx_timeout((uart_port_t)rs485_cfg.uart_num, 3));
//uint8_t* data = (uint8_t*) malloc(BUF_SIZE);
//echo_send(rscfg->uart_num, "Start RS485 Output test.\r\n", 24);
char * bff = (char *)malloc(10);
uint8_t count = 0;
while (rep)
{ 
  sprintf(bff,"%02d",counter++);
  if (counter>98) counter=0; 
  //echo_send(rscfg->uart_num, "A", 1);
  uart_write_bytes((uart_port_t)rs485_cfg.uart_num, bff, strlen(bff));
  uart_wait_tx_done((uart_port_t)rs485_cfg.uart_num, 10);

    ESP_LOGI("TOOL","%s", bff);
    fflush(stdout);

    if(count++>10) {
      count=0;
      //printf("\n>> ");fflush(stdout);
    }

  vTaskDelay(5/portTICK_PERIOD_MS);

        
      }
}

void rs485_input_test(gpio_num_t led)
{
//bool rep = true;
//uint16_t counter = 0;

int BAUD = 115200;

RS485_config_t rs485_cfg={};
rs485_cfg.uart_num = 1;
rs485_cfg.dev_num  = 253;
rs485_cfg.rx_pin   = 25;
rs485_cfg.tx_pin   = 26;
rs485_cfg.oe_pin   = 13;
rs485_cfg.baud     = BAUD;

ESP_LOGW("TOOL","RS485 Input Test");

uart_driver_delete((uart_port_t)rs485_cfg.uart_num);

        uart_config_t uart_config = {};
        uart_config.baud_rate = BAUD;
        uart_config.data_bits = UART_DATA_8_BITS;
        uart_config.parity = UART_PARITY_DISABLE;
        uart_config.stop_bits = UART_STOP_BITS_1;
        uart_config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
        uart_config.rx_flow_ctrl_thresh = 122;

ESP_ERROR_CHECK(uart_driver_install((uart_port_t)rs485_cfg.uart_num, BUF_SIZE * 2, 0, 0, NULL, 0));
ESP_ERROR_CHECK(uart_param_config((uart_port_t)rs485_cfg.uart_num, &uart_config));
ESP_ERROR_CHECK(uart_set_pin((uart_port_t)rs485_cfg.uart_num, rs485_cfg.tx_pin, rs485_cfg.rx_pin, rs485_cfg.oe_pin, UART_PIN_NO_CHANGE));
ESP_ERROR_CHECK(uart_set_mode((uart_port_t)rs485_cfg.uart_num, UART_MODE_RS485_HALF_DUPLEX));
ESP_ERROR_CHECK(uart_set_rx_timeout((uart_port_t)rs485_cfg.uart_num, 3));

char * bff = (char *)calloc(1,10);
uint8_t count = 0;

while (true)
{ 
  
  int len = uart_read_bytes((uart_port_t)rs485_cfg.uart_num, bff, 2, (100 / portTICK_PERIOD_MS));
  
  if (len > 0) {
    gpio_set_level(led,1);
    printf("%s ", bff);
    fflush(stdout);
    if(count++>10) {
      count=0;
      printf("\n>> ");fflush(stdout);
    }
    //vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(led,0);
  } else ESP_ERROR_CHECK(uart_wait_tx_done((uart_port_t)rs485_cfg.uart_num, 10));
   

}

}