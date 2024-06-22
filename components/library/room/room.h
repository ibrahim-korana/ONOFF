/*
 * room.h
 *
 *  Created on: 10 Nis 2023
 *      Author: korana
 */

#ifndef _ROOM_H_
#define _ROOM_H_

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

//Lamba Version 1.1

class Room : public Base_Function {
    public:
      Room(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"room");
        genel.oid = 1;
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
      ~Room() {};
    void set_status(home_status_t stat);
    void remote_set_status(home_status_t stat, bool callback_call=true);

		void get_status_json(cJSON* obj) override;
    bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 
		
		void init(void);

		//void fire(bool stat);
		void ConvertStatus(home_status_t stt, cJSON* obj);
		//void senaryo(char *par) override;

    private:
		
		esp_timer_handle_t ytimer = NULL;

	protected:
	  static void func_callback(void *arg, port_action_t action);
	  static void xtim_callback(void* arg);
	  void ytim_stop(void);
	  void ytim_start(void);
	  bool ytim_is_active(void) {return esp_timer_is_active(ytimer);}
    bool local_on = false;
	  
    static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);



    };


#endif
