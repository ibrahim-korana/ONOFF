#ifndef _PLED_H
#define _PLED_H

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

//PwmLED Version 1.0

class PwmLed : public Base_Function {
    public:
	PwmLed(int id, function_callback_t cb, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"pled");
        function_callback = cb;
        disk = dsk;
        disk.read_status(&status,genel.device_id);
        local_port_callback = &func_callback;
        if (!status.first) {             
              memset(&status,0,sizeof(status));
              status.stat     = false;
              status.active   = true;
              status.first    = true;
              status.status   = 0;
              status.color    = 0;
              write_status();
        }      
      };
      ~PwmLed() {};

      /*fonksiyonu yazılımla aktif hale getirir*/
      void set_status(home_status_t stat);  
      void set_color(uint16_t color);
      void remote_set_status(home_status_t stat, bool callback_call=true);
           
      void get_status_json(cJSON* obj) override;
     // void room_event(room_event_t ev) override;
      //bool get_port_json(cJSON* obj) override;
      void init(void);

    //  void fire(bool stat);
      void ConvertStatus(home_status_t stt, cJSON* obj);

      uint8_t sure = 0;
      void xtim_stop(void);
      void xtim_start(void);  
    //  void senaryo(char *par) override;
        
    private:
      esp_timer_handle_t qtimer = NULL;
      esp_timer_handle_t xtimer = NULL;
      
    protected:  
      static void func_callback(void *arg, port_action_t action);     
      static void lamp_tim_callback(void* arg);    
      static void xtim_callback(void* arg);      
      void tim_stop(void);
      void tim_start(void);

};

#endif
