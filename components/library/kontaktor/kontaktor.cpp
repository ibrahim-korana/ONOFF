#include "kontaktor.h"
#include "esp_log.h"

static const char *KONT_TAG = "KONTAKTOR";

//bu fonksiyon prizi yazılım ile tetiklemek için kullanılır.
void Contactor::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(status.stat);
                }
            target = target->next;
        }
        write_status();
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Contactor), portMAX_DELAY));
    } else {
        bool chg = false;
        if (status.stat!=stat.stat) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(KONT_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Contactor), portMAX_DELAY));
        }      
    }
}

void Contactor::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
}

void Contactor::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


void Contactor::tim_callback(void* arg)
{   
    Contactor *aa = (Contactor *)arg;
    home_status_t ss = aa->get_status();
    ss.stat = false;
    aa->set_status(ss);
}

void Contactor::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Contactor::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}

void Contactor::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Contactor *con = (Contactor *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;   
    if (dev_id==con->genel.device_id || id==ROOM_ON || id==ROOM_OFF || id==ROOM_FON) {
        if (dev_id>0) {
            //Status var bunu kullan
            con->set_status(*st);     
        } else {  
                if (id==ROOM_ON) con->room_on = true;
                if (id==ROOM_FON) {
                                     con->room_on = true;
                                     //odaya fiziki olarak girildi kontrol et ve çalıştır.
                                     if (con->room_cont)
                                       {
                                            home_status_t ss = con->get_status();
                                            ss.stat = true;
                                            con->set_status(ss); 
                                       }                                     
                                  }
                if (id==ROOM_OFF) {
                    con->room_on = false;
                    if (con->room_cont)
                        {
                            home_status_t ss = con->get_status();
                            ss.stat = false;
                            con->set_status(ss);  
                        }                  
                }  
        }
    } 
}

//Alarm mesajları
void Contactor::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Contactor *con = (Contactor *) handler_args;
    if (st->stat) {
        //alarm deprem mesajı alındı kontaktoru gecikmeli kapat
        con->store_set_status();
        con->tim_start();
    } else {
        con->store_get_status();
        con->tim_stop();
        home_status_t ss = con->get_status();
        con->set_status(ss); 
    }  
}


void Contactor::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=120;
        if ((global & 0x01) == 0x01) room_cont = true;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        set_status(status);       
    }
}


/*
       BETA TEST OK
       
       Kontaktor, kendisine tanımlanan roleyi yazılım ile açıp kapatan bir objedir. 
       olası tanımlama:
       {
            "name":"cont",
            "uname":"kontaktor",
            "id": 5,
            "loc": 1,
            "timer" :80,
            "global":0,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 5, "pcf":1, "name":"Kontaktor","type":"PORT_OUTPORT"}
                ]
            }
        }

            Objeye en az bir çıkış portu tanımlamak zorunludur. Tanımlama içinde timer
        yangın veya deprem anında kontaktorun kapatma gecikme süresini tanımlar. 
            Global 1 ise kontaktör oda rolesine baglı olarak çalışır. Odanın kapanması ile 
        kapatılır, odaya fiziksel olarak girilmesi ile açılır.

*/