/*
 * room.h
 *
 *  Created on: 10 Nis 2023
 *      Author: korana
 */

#ifndef _KLIMA_H_
#define _KLIMA_H_

#include <stdio.h>
#include <string.h>
#include "core.h"
#include "storage.h"
#include "classes.h"
#include "esp_timer.h"

class Klima : public Base_Function {
    public:
      Klima(int id, Storage dsk) {
        genel.device_id = id;
        strcpy(genel.name,"klima");
        genel.oid = 7;
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
      ~Klima() {};
        void set_status(home_status_t stat);
      	//void remote_set_status(home_status_t stat, bool callback_call=true);

		void get_status_json(cJSON* obj) override;
    bool get_port_json(cJSON* obj) override
      {
          return false;
      }; 
		void init(void);
		void ConvertStatus(home_status_t stt, cJSON* obj);
		

    private:
      esp_timer_handle_t qtimer = NULL;
      esp_timer_handle_t remote_timer = NULL;
      bool remote_start=false;
      uint8_t remote_temp=0;
      uint8_t remote_set_temp=29;


	protected:
	  static void func_callback(void *arg, port_action_t action);
    uint64_t bit_set(uint64_t number, uint8_t n);
    uint64_t bit_clear(uint64_t number, uint8_t n);
    uint64_t bit_status(uint64_t number, uint8_t n, uint8_t val);
    void rapor_create(void);
    static void tim_callback(void* arg);
    void rate(home_status_t st);
    void rate(void);
    void printBits(size_t const size, void const * const ptr);
    void tim_stop(void);
    void tim_start(void);  
    void remote_tim_stop(void);
    void remote_tim_start(void); 

    static void in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
    static void alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
    static void virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
    };


#endif
