
#include "RoomAlarm.h"
#include "esp_log.h"

static const char *ROOMALARM_TAG = "ROOMALARM";

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void RoomAlarm::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        if (!stat.stat) stat.status=0;
        local_set_status(stat);
        genel.active = status.active;
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(RoomAlarm), portMAX_DELAY));
        if (status.status==1) {
            home_message_t mm = {};
            mm.txt = NULL;
            mm.id = 0;
            ESP_ERROR_CHECK(esp_event_post(MESSAGE_EVENTS, MESSAGE_ALARM, &mm, sizeof(mm), portMAX_DELAY));
            vTaskDelay(50/portTICK_PERIOD_MS); 
        }
    } else {
        bool chg = false;
        if (status.stat!=stat.stat) chg=true;
        if (status.active!=stat.active) chg=true;
        
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(ROOMALARM_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(RoomAlarm), portMAX_DELAY));
        }      
    }
}

void RoomAlarm::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj,"status",status.status);
    cJSON_AddNumberToObject(obj, "oid", genel.oid);
}

void RoomAlarm::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


void RoomAlarm::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    RoomAlarm *emg = (RoomAlarm *) handler_args;
    uint8_t dev_id = 0, sta=0,stt=0;
    if (id==MOVEMEND) return;
    if (st!=NULL) {dev_id = st->id; sta=st->stat;stt=st->status;}   
    if (dev_id==emg->genel.device_id || id==CHECK_IN || id==CHECK_OUT) {
        if (dev_id>0) {
            //Status var bunu kullan
                printf("in_handler status %d %d %d\n",dev_id, sta,stt); 
                emg->set_status(*st);     
                      }  else {
                         if (id==CHECK_IN || id==CHECK_OUT)
                          {
                             home_status_t ss = emg->get_status();
                             ss.stat = false;
                             ss.status = 0;
                             emg->set_status(ss);
                          }
                      }
    }
}

void RoomAlarm::movemend_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    Base_Function *st = (Base_Function *) event_data;
    RoomAlarm *emg = (RoomAlarm *) handler_args;
    home_status_t ss = emg->get_status();
   // printf("%s\n",st->genel.name);
   //printf("movemend_handler status %d %d %s\n",ss.stat , ss.status, st->genel.name);
    if (ss.stat && ss.status==0) {
        bool chg = false; 
        if (strcmp(st->genel.name,"dnd")==0) chg=true;
        if (strcmp(st->genel.name,"cln")==0) chg=true;
        if (strcmp(st->genel.name,"oda")==0) chg=true;
        if (strcmp(st->genel.name,"lamp")==0) chg=true;
        if (strcmp(st->genel.name,"klima")==0) chg=true;
        if (chg) {
           ss.status = 1;
           emg->set_status(ss);
        }        
    }
    
}


void RoomAlarm::init(void)
{
    if (!genel.virtual_device)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, MOVEMEND, movemend_handler, (void *)this, NULL));
        set_status(status);
    }
}

/*
       
       Roomalarm objesi oda içinde herhangi bir faaliyet olduğunda alarm üretmek için üzere düzenlenmiş bir objedir.
       Herhangi bir çıkış veya giriş portu bulunmaz.  

       Olası tanım:

       {
            "name":"ralarm",
            "uname":"ODA ALARMI",
            "id": 9,
            "loc": 0,
            "timer": 0,
            "global": 0
            "hardware": {
                "location" : "local",
                "port": [ ]
            }
        }   
*/