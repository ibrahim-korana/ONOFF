#ifndef _CHKIN_H
#define _CHKIN_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class Checkin : public Base_Function {
    public:
      Checkin(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"checkin");
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
      ~Checkin() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);      
      void get_status_json(cJSON* obj) override;
      void init(void) override;
      void ConvertStatus(home_status_t stt, cJSON* obj);
      bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 

      char *get_misafir(void) {return (char*)mis_name;};
        
    private:
      uint32_t in_date;
      uint32_t out_date;  
      uint8_t mis_name[16]; 
      uint8_t auto_checkout;
      uint8_t temp=0;
      uint8_t set_temp=35;
      
    protected:        
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);      
};

#endif