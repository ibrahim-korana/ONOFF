#ifndef _SECURITY_H
#define _SECURITY_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

typedef enum {
    SEC_CLOSED = 0,
    SEC_PROCESS,
    SEC_OPENED,
    SEC_UNKNOWN, 
    SEC_ALARM,
    SEC_STOP,
} security_status_t;

class Security : public Base_Function {
    public:
      Security(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"sec");
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
      ~Security() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);
      void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
      bool get_port_json(cJSON* obj) override;
      void get_port_status_json(cJSON* obj) ;
      void init(void) override;

      void alarm_close(void);
      void alarm_open(void);
      void alarm_stop(void);
      void tim_stop(void);
      void tim_start(void);  
      void cikis_port_active(void);

      void fire(bool stat);
      void ConvertStatus(home_status_t stt, cJSON* obj);
        
    private:
      esp_timer_handle_t qtimer = NULL;
      
    protected:  
      static void func_callback(void *arg, port_action_t action); 
      static void tim_callback(void* arg);     
      static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
      static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);         
};

#endif