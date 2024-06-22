#ifndef _BELL_H
#define _BELL_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class Bell : public Base_Function {
    public:
      Bell(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"bell");
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = true;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              write_status();
        } 
        genel.active = status.active;     
      };
      ~Bell() {};

      void status_change(bool st);

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);               
      void get_status_json(cJSON* obj) override;
      void init(void) override;

      void ConvertStatus(home_status_t stt, cJSON* obj);
      bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 
        
    private:
      esp_timer_handle_t qtimer = NULL;
      void tim_stop(void);
      void tim_start(void);
    protected:  
      //static void func_callback(void *arg, port_action_t action); 
      bool bell_alarm =false;
      static void tim_callback(void* arg);    
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data); 
      static void func_callback(void *arg, port_action_t action);
      uint8_t fire_count = 0;
      bool room_on = false;
};

#endif