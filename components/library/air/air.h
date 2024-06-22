#ifndef _AIR_H
#define _AIR_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "jsontool.h"


class Air : public Base_Function {
    public:
      Air(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"air");
        disk = dsk;
        disk.read_status(&status,genel.device_id);
                
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = true;
              status.set_temp = 25;
              status.temp     = 26;
              status.active   = true;
              status.first    = true;
              status.counter  = 0;
              write_status();
        } 
        genel.active = status.active;       
      };
      ~Air() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);      
      //void set_sensor(char *name, home_status_t stat);
           
      void get_status_json(cJSON* obj) override;
          bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 
      void init(void) override;

      //void fire(bool stat);
      //void senaryo(char *par);
      //void set_start(bool drm); 
      void ConvertStatus(home_status_t stt, cJSON* obj);
        
    private:
    protected:   
      void temp_action(bool send=true);    
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
      static void alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
};

#endif