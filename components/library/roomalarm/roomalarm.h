#ifndef _ROOMALARM_H
#define _ROOMALARM_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class RoomAlarm : public Base_Function {
    public:
      RoomAlarm(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"ralarm");
        genel.oid = 5;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status = 0;
              write_status();
        } 
        genel.active = status.active;     
      };
      ~RoomAlarm() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);    
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 
      void init(void) override;

      void ConvertStatus(home_status_t stt, cJSON* obj);
        
    private:
      
    protected:          
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void movemend_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) ;    
};

#endif