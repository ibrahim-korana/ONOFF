#ifndef _LAMPON_H
#define _LAMPON_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class Lampon : public Base_Function {
    public:
      Lampon(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"lampon");
        disk = dsk;
        local_port_callback = &func_callback;
        disk.read_status(&status,genel.device_id);
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
      ~Lampon() {};

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
      uint8_t room_on=false;
      esp_timer_handle_t qtimer = NULL;
      static void tim_callback(void* arg);
      void tim_stop(void);
      void tim_start(void);
      
    protected:        
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data); 
      static void func_callback(void *arg, port_action_t action);  
            
};

#endif