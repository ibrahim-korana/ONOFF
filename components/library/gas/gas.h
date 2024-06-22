#ifndef _GAZ_H
#define _GAZ_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"


class Gas : public Base_Function {
    public:
      Gas(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"gas");
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
      ~Gas() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);
      void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
      void init(void) override;
      void ConvertStatus(home_status_t stt, cJSON* obj);
      bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 
        
    private:
      bool room_water=false;
      uint8_t kapatma_suresi = 15;
      uint8_t acma_suresi = 15;
      bool motorlu = 0;

      void set_motorlu(bool drm);
      void set_motorsuz(bool drm);
      esp_timer_handle_t qtimer;
      void tim_start(uint8_t tm);
      
    protected:       
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);  
      static void timer_callback(void* arg); 
};

#endif