#ifndef _DND_H
#define _DND_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class Dnd : public Base_Function {
    public:
      Dnd(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"dnd");
        genel.oid = 2;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = 0;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              status.color    = 0;
              write_status();
        } 
        genel.active = status.active;
        anahtarlar = (status.color &0x00F0)>>8;
        ledler = status.color & 0x000F;
      };
      ~Dnd() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);      
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void init(void) override;
      void ConvertStatus(home_status_t stt, cJSON* obj);
        
    private:
      esp_timer_handle_t clean_timer = NULL;
      esp_timer_handle_t dnd_timer = NULL; 
      
    protected:  
      static void func_callback(void *arg, port_action_t action);  
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);    

      void input_rate(void);
      void output_rate(void);
      uint8_t bit_set(uint8_t number, uint8_t n);
      uint8_t bit_clear(uint8_t number, uint8_t n); 
      uint8_t bit_get(uint8_t number, uint8_t n); 
      uint8_t anahtarlar = 0;
      uint8_t ledler = 0;

      void dnd_timer_stop(void);
      void dnd_timer_start(void);
      void clean_timer_stop(void);
      void clean_timer_start(void);
      static void dnd_timer_callback(void* arg);   
      static void clean_timer_callback(void* arg); 

      uint8_t dnd_duration = 10;
      uint8_t clean_duration = 10;
};

#endif