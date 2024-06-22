
#include "Fire.h"
#include "esp_log.h"

static const char *FIRE_TAG = "FIRE";

void Fire::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port; //hangi buton action üretti
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn); //butonun portu ne
        button_event_t evt = iot_button_get_event(btn); //action eventi ne
        Fire *obj = (Fire *) arg; //ben kimim
        home_status_t stat = obj->get_status(); //Benim statusum ne?
        bool change = false;
        //obje iptal degilse ve event up veya down ise
        
        if ((evt==BUTTON_PRESS_DOWN) && obj->genel.active)
           {               
                     if (prt->type==PORT_INPORT || prt->type==PORT_INPULS) change = true;
                     
           }
        if (change & !stat.stat) {
            ESP_LOGW(FIRE_TAG,"Duman Algılandı");
            stat.stat = true;
            obj->local_set_status(stat,true);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, FIRE_ON, (void *)obj, sizeof(Fire), portMAX_DELAY));
            ESP_LOGI(FIRE_TAG,"REAL FIRE START");
            home_message_t mm = {};
                mm.txt = NULL;
                mm.id = 0;
                ESP_ERROR_CHECK(esp_event_post(MESSAGE_EVENTS, MESSAGE_FIRE, &mm, sizeof(mm), portMAX_DELAY));
                vTaskDelay(50/portTICK_PERIOD_MS); 
            obj->tim_start();
        } 
     }
}


//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Fire::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        genel.active = status.active;
        if (status.stat) {
                ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, FIRE_ON, (void *)this, sizeof(Fire), portMAX_DELAY));
                tim_start();
                ESP_LOGI(FIRE_TAG,"STATUS FIRE START");
            } else {
                ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, FIRE_OFF, (void *)this, sizeof(Fire), portMAX_DELAY));
                tim_stop();
                ESP_LOGI(FIRE_TAG,"FIRE STOP");
            }
    } else {
        bool chg = false;
        if (status.stat!=stat.stat) chg=true;
        if (status.active!=stat.active) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(FIRE_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Fire), portMAX_DELAY));
        }      
    }
}

void Fire::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "oid", genel.oid);
}

void Fire::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


void Fire::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Fire *fr = (Fire *) handler_args;
    uint8_t dev_id = 0, stt=0;
    if (id==MOVEMEND) return;
    if (st!=NULL) {dev_id = st->id;  stt=st->stat;}  
    if (dev_id==fr->genel.device_id && id!=MOVEMEND) {
        if (dev_id>0) {
                //Status var bunu kullan
                 printf("fire in handler %d  %d %d\n",dev_id, fr->genel.device_id, stt);
                 if (stt<2 ) fr->set_status(*st);     
                      }  else {
                            //
                      }             
    }
}


//Sensor mesajları
void Fire::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Fire *fr = (Fire *) handler_args;
    Base_Port *target = fr->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {        
                printf("virtual handler\n") ;       
                if (st->stat==1 && fr->genel.active)
                  {
                      home_status_t ss = fr->get_status();
                      ss.stat = 1;
                      fr->set_status(ss);
                  }
            }
        target=target->next;
    }
}

void Fire::tim_callback(void* arg)
{   
    Fire *aa = (Fire *)arg;
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, FIRE_ON, (void *)aa, sizeof(Fire), portMAX_DELAY));
    ESP_LOGI(FIRE_TAG,"FIRE ACTIVE");
}

void Fire::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Fire::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_periodic(qtimer, duration * 1000000));
}

void Fire::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=60;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        set_status(status);
    }
}

/*

       Fire objesi oda içine takılacak duman veya alev sensorunden gelen yangın mesajını bildirmek üzere düzenlenmiş bir objedir.
       Herhangi bir çıkış portu bulunmaz.  Resepsiyonda alarma neden olur. 
       virtual veya fiziksel Giriş portu tanımlanabilir.

       Olası tanım:

       {
            "name":"fire",
            "uname":"YANGIN",
            "id": 9,
            "loc": 0,
            "timer": 5,
            "global": 0
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 10, "pcf":1, "name":"acil1","type":"PORT_INPORT"},
                    {"pin": 0, "pcf":0, "name":"AN09","type":"PORT_VIRTUAL"},
                ]
            }
        }

        timer, fire aktif oldugunda durduruluncaya kadar kaç sn bir alarm üretecegini belirler    

*/