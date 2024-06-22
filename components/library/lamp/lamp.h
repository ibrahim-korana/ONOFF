#ifndef _LAMP_H
#define _LAMP_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

//Lamba Version 1.1

class Lamp : public Base_Function {
    public:
      Lamp(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"lamp");
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              write_status();
        }  
        genel.active = status.active;  
      };
      ~Lamp() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat); 
      void set_status_non_message(home_status_t stat); 
     // void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
     // void room_event(room_event_t ev) override;
      bool get_port_json(cJSON* obj) override;
      void init(void);

      void ConvertStatus(home_status_t stt, cJSON* obj);

      void flash_on(void);
      void flash_off(void);

      uint8_t sure = 0;
      void xtim_stop(void);
      void xtim_start(void);  
      
        
    private:
      esp_timer_handle_t qtimer = NULL;
      esp_timer_handle_t xtimer = NULL;
      esp_timer_handle_t ftimer = NULL;
      
      
    protected:  
      static void func_callback(void *arg, port_action_t action); 
      void send_virtual_anahtar(uint8_t sender);    
      static void lamp_tim_callback(void* arg);    
      static void xtim_callback(void* arg);   
      static void ftim_callback(void* arg);    
      void tim_stop(void);
      void tim_start(void);
      bool room_lamp = false;
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
      static void alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
      home_status_t temp_status;
      bool store_stat=false;
      uint8_t fire_count = 0;

};

#endif
