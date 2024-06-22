#ifndef _DAYCLEAN_H
#define _DAYCLEAN_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class DayClean : public Base_Function {
    public:
      DayClean(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"dayclean");
        genel.oid = 3;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              for (int i=0;i<16;i++) {status.ircom[i]=0;status.irval[i]=0;}
              write_status();
        } 
        genel.active = status.active;
      };
      ~DayClean() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);      
      void get_status_json(cJSON* obj) override;
      void init(void) override;
      void ConvertStatus(home_status_t stt, cJSON* obj);

      void t1_conv(uint8_t *dest, struct tm *sourge);
      void t2_conv(uint8_t *dest, struct tm *sourge);

      tm getTimeStruct();
      bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 
        
    private:
      struct tm start_time;
      struct tm end_time;
      
    protected:        
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);     
};

#endif