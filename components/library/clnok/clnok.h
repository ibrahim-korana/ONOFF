#ifndef _CLNOK_H
#define _CLNOK_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class ClnOK : public Base_Function {
    public:
      ClnOK(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"clnok");
        genel.oid = 4;
        disk = dsk;
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
      ~ClnOK() {};

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
      static void tim_callback(void* arg);
      void tim_stop(void);
      void tim_start(void);
      
    protected:        
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);      
};

#endif