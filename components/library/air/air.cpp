#include "air.h"
#include "esp_log.h"

static const char *AIR_TAG = "AIR";

ESP_EVENT_DEFINE_BASE(VIRTUAL_SEND_EVENTS);

void Air::temp_action(bool send)
{
    bool change = false, durum = false;
    if (global==0)
    {
        //Isıtma Modu
        if (status.temp>=status.set_temp) {
                                    durum=false;
                                    change=true;
                                } else {
                                        durum = true;
                                        change = true;
                                        }
    }
    if (global==1)
    {
        //Sogutma Modu
        if (status.temp>0)
        {
            if (status.temp<=status.set_temp) {
                                        durum=false;
                                        change=true;
                                    } else {
                                            durum = true;
                                            change = true;
                                            }
        }
    }

    if (status.active==true && status.counter>0) {
                                        durum = true;
                                        change = true;
                                                  }                                 
    if (!status.stat) {durum=false;change=true;} 
    if (!status.active) {durum=false;change=true;} 
    if (change)
    {                                
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) status.status = target->set_status(durum);
            target = target->next;
        }
    }  
    if (send) ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Air), portMAX_DELAY));
}


void Air::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        status.stat = stat.stat;
        status.active = stat.active;
        if (stat.active!=genel.active) genel.active = stat.active;
        if (!genel.active) status.stat=false;
        status.counter = stat.counter;
        status.temp = stat.temp;
        if (stat.set_temp!=status.set_temp) 
        {
            status.set_temp = stat.set_temp;
            if (stat.first) {
                printf("termostata söyle\n");
                status.first = false;
                stat.first = false;
                //Virtual portları bul
                Base_Port *target = port_head_handle;
                while (target) {
                    if (target->type == PORT_VIRTUAL)
                        {                             
                           home_virtual_t ss = {};
                           strcpy((char*)ss.name,target->name);
                           ss.set_temp = status.set_temp;
                           ss.temp = status.temp;
                           ss.sender= 255;
                           ss.stat = status.stat;
                           ss.active = status.active;
                           ESP_ERROR_CHECK(esp_event_post(VIRTUAL_SEND_EVENTS, VIRTUAL_SEND_DATA, (void *)&ss, sizeof(home_virtual_t), portMAX_DELAY));
                        }  
                    target=target->next;
                }                
            }
        } 
        write_status();
        temp_action(false);
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Air), portMAX_DELAY));
    } else {
        bool chg = false;
        if (status.active!=stat.active) chg=true;
        if (status.stat!=stat.stat) chg=true;
        if (status.counter!=stat.counter) chg=true;
        if (status.set_temp!=stat.set_temp) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(AIR_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Air), portMAX_DELAY));
        } 
    }
}


void Air::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    if (stt.counter>0) cJSON_AddNumberToObject(obj, "coun",1); else cJSON_AddNumberToObject(obj, "coun",0);
    cJSON_AddNumberToObject(obj, "status", stt.status);
    char *mm = (char *)malloc(10);
    sprintf(mm,"%2.02f",stt.temp); 
    cJSON_AddNumberToObject(obj, "temp", atof(mm));
    sprintf(mm,"%2.02f",stt.set_temp);
    cJSON_AddNumberToObject(obj, "stemp", atof(mm));
    free(mm);
}

void Air::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


//yangın bildirisi aldığında ne yapacak
/*
void Air::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        //ısıtma yangın ihbarında kapatılır
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    status.status = target->set_status(false);
                }
            target = target->next;
        }
    } else {
       status = main_temp_status;
       Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(status.status);
                }
            target = target->next;
        }      
    }
}

void Air::senaryo(char *par)
{
    cJSON *rcv = cJSON_Parse(par);
    if (rcv!=NULL)
    {
        JSON_getfloat(rcv,"temp",&(status.temp));
        JSON_getbool(rcv,"stat",&(status.stat));
        set_status(status);
        cJSON_Delete(rcv);  
    } 
}
*/

void Air::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Air *klm = (Air *) handler_args;
    Base_Port *target = klm->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                             
                     home_status_t ss = klm->get_status();
                     ss.temp = st->temp;
                     ss.set_temp = st->set_temp;
                     klm->set_status(ss);  
            }  
        target=target->next;
    }
}

//Alarm mesajları Deprem veya yangında air durdurulur
void Air::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    Air *klm = (Air *) handler_args;
    home_virtual_t *st = (home_virtual_t *) event_data;
    if (st->stat) {
        klm->store_set_status();
        home_status_t ss = klm->get_status();
        ss.stat=false;
        klm->set_status(ss);
    } else {
        klm->store_get_status();
        home_status_t ss = klm->get_status();
        klm->set_status(ss); 
    }     
}

void Air::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Air *klm = (Air *) handler_args;
    if (id==MOVEMEND) return;
    uint8_t dev_id = 0;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==klm->genel.device_id) {
        if (dev_id>0) {
            //Status var bunu kullan
            st->first = true;
            klm->set_status(*st);     
        } 
    } 
}

void Air::init(void)
{
    if (!genel.virtual_device)
    {
        if (global==0) ESP_LOGI(AIR_TAG,"TERMOSTAT ISITMA MODUNDA");
        if (global==1) ESP_LOGI(AIR_TAG,"TERMOSTAT SOGUTMA MODUNDA");
        temp_action(false);
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL));
    }
}


/*
      Air objesi termostat olarak çalışır. 
      Isıtma ve sogutma olmak üzere 2 ayrı algoritma ile kendisine baglanan çıkış rölesini kontrol eder. 
      
      Obje ister sogutma isterse ısıtma olarak tanımlanmış olsun, Auto ve Manuel olmak üzere 2 ayrı modda sahiptir.
      Auto modda sensorden gelen ısı degeri ve set degeri dikkate alınarak çıkış rölesi açılıp kapatılırken 
      manuel modda kullanıcıdan gelen aç/kapat emirlerine göre açılıp kapatılır. 

      Objeye en az 1 çıkış rölesi tanımlanmak zorundadır. Olası tanım:

      {
            "name":"air",
            "uname":"Isıtma",
            "id": 1,
            "loc": 0,     
            "timer": 0,
            "global":0,
            "hardware": {
                "port": [
                    {"pin": 0, "pcf":0, "name":"SN04", "type":"PORT_VIRTUAL"},
                    {"pin": 4, "pcf":1, "name":"KLM role", "type":"PORT_OUTPORT"}
                ]
            }
      }

      global degeri ısıtma/sogutma olarak çalışacagını belirler. Global degeri 0 ise ısıtma,
      1 ise sogutma algoritmaları çalışır.
      
      Objeye Virtual bir sensor tanımlanmak zorundadır. Bu sensor mevcut ısı, set ve manuel bilgilerini 
      gönderecektir. 

      Air objesi oda bilgilerini dikkate almaz. 

*/