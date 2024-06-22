
#include "checkin.h"
#include "esp_log.h"

static const char *CHECKIN_TAG = "CHECKIN";

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Checkin::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);  
        if (status.stat)
          {
            in_date = (uint32_t) stat.temp;
            out_date = (uint32_t) stat.set_temp;
            memset(mis_name,0,16);
            strcpy((char*)mis_name,(char*)stat.ircom);  
            auto_checkout = stat.status;
            ESP_LOGI(CHECKIN_TAG,"Check/in Action %s",mis_name);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, CHECK_IN, (void *)this, sizeof(Checkin), portMAX_DELAY));
          } else {
            in_date = 0;
            out_date= 0;
            auto_checkout = 0;
            strcpy((char*)mis_name,"");
            ESP_LOGI(CHECKIN_TAG,"Check/out Action");
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, CHECK_OUT, (void *)this, sizeof(Checkin), portMAX_DELAY));
          }                                                                                                                                                   
        write_status();
    } else {
        bool chg = local_set_status(stat,true);
        if (chg)
        {
            ESP_LOGI(CHECKIN_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Checkin), portMAX_DELAY));
        }      
    }
}


//statusu json olarak döndürür
void Checkin::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    cJSON_AddNumberToObject(obj,"temp",in_date);
    cJSON_AddNumberToObject(obj,"stemp",out_date);
    cJSON_AddStringToObject(obj,"ircom",(char*)mis_name);
    cJSON_AddNumberToObject(obj,"status",auto_checkout);
}

void Checkin::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

//Sensor mesajları
void Checkin::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Checkin *obj = (Checkin *) handler_args;
    Base_Port *target = obj->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                
                //Gelen ısı bilgisini kaydet
                obj->temp = st->temp;
            }
        target=target->next;
    }
}


//Yazılım ile gelen eventler
void Checkin::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Checkin *con = (Checkin *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==con->genel.device_id) {
        if (dev_id>0) {
            //Status var bunu kullan           
            con->set_status(*st);     
        } 
    } 
}

void Checkin::init(void)
{
    if (!genel.virtual_device)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
      //  ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        set_status(status);
    }
}


/*
    Odaya giren misafiti takip için  kullanılan bir objedir.
    olası Tanım :

    {
            "name":"checkin",
            "uname":"Misair",
            "id": 5,
            "loc": 1,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 0, "pcf":0, "name":"SN01","type":"PORT_VIRTUAL"}
                ]
            }
        }

    Açıp kapatma işlemi remote olarak tanımlanabilir. 

    Checkin/out status.stat ile gelir
    chekin tarihi temp,
    checkout tarihi stemp,
    isim ircom
    auto_checkout ise status.status üserinden gelecektir.  
    

    checkin oldugunda klima 5dk süre ile çalıştırılır sonra normal çalışmasına döner.
    cleanok kapatılır.


    checkout oldugunda dnd,teknik,minibar,clean,cleanok kapatılır. 

    checkin/Checkout yapıldığında room alarm kapatılır.  

    Checkin ısıyı takip eder. Bu nedenle Virtual ısı sensoru tanımı yapılabilir. 

  
*/

