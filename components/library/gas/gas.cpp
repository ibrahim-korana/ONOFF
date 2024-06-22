#include "gas.h"
#include "esp_log.h"

static const char *KONT_TAG = "GAZ";

void Gas::timer_callback(void* arg)
{
    Gas *mthis = (Gas *)arg;
    Base_Port *target = mthis->port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(false);
                }
            target = target->next;
        }
    mthis->status.status = 0;  
    mthis->write_status();
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)mthis, sizeof(Gas), portMAX_DELAY));
}

void Gas::set_motorlu(bool drm)
{
    char *cc = (char *)calloc(1,5);
    if (drm) strcpy(cc,"ON"); else strcpy(cc,"OFF");
    Base_Port *target = port_head_handle;
    while (target) {
        if (target->type == PORT_OUTPORT && strcmp(target->name,cc)==0) 
            {
                target->set_status(true);
            }
        target = target->next;
    }
    status.status = 1;
    uint8_t sr = (drm)?acma_suresi:kapatma_suresi;
    free(cc);
    tim_start(sr);
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Gas), portMAX_DELAY));
    ESP_LOGI(KONT_TAG,"GAZ PORT ACTION");
}

void Gas::set_motorsuz(bool drm)
{
    Base_Port *target = port_head_handle;
    while (target) {
        if (target->type == PORT_OUTPORT) 
            {
                target->set_status(drm);
            }
        target = target->next;
    }
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Gas), portMAX_DELAY));
    ESP_LOGI(KONT_TAG,"GAZ PORT ACTION");
}

void Gas::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        bool chg = local_set_status(stat);
    
        if (stat.active!=genel.active) genel.active = stat.active;
        if (!genel.active) status.active=false;
        if (genel.active && chg)
        {
            if (motorlu) {
                set_motorlu(status.stat);
            } else {
                set_motorsuz(status.stat); 
            }
            write_status();
        }
    } else {
        bool chg = false;
        if (status.stat!=stat.stat) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(KONT_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Gas), portMAX_DELAY));
        }      
    }
}

void Gas::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj,"status",status.status);
}

void Gas::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


void Gas::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Gas *con = (Gas *) handler_args;
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
                                     if (con->room_water)
                                       {
                                            home_status_t ss = con->get_status();
                                            ss.stat = true;
                                            con->set_status(ss); 
                                       }                                     
                                  }
                if (id==ROOM_OFF) {
                    con->room_on = false;
                    if (con->room_water)
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
void Gas::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //Deprem veya yangında gaz kesilir.
    home_virtual_t *st = (home_virtual_t *) event_data;
    Gas *con = (Gas *) handler_args;
    if (st->stat) {
        //alarm deprem mesajı alındı gazı kapat
        con->store_set_status();
        home_status_t ss = con->get_status();
        ss.stat=false;
        con->set_status(ss);
    } else {
        con->store_get_status();
        home_status_t ss = con->get_status();
        con->set_status(ss); 
    }  
}

void Gas::tim_start(uint8_t tm)
{
  if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
  ESP_ERROR_CHECK(esp_timer_start_once(qtimer, tm * 1000000));
}

void Gas::init(void)
{
    if (!genel.virtual_device)
    {
        if ((global & 0x01) == 0x01) motorlu = true;
        if ((global & 0x02) == 0x02) room_water = true;
        if ((global>>2)>0) kapatma_suresi = (global>>2);
        if (duration>0) acma_suresi = duration;
        esp_timer_create_args_t arg = {};
        arg.callback = &timer_callback;
        arg.name = "tim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 

        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        set_status(status);       
    }
}


/*
       Beta TEST
          Motorsuz  OK
          Notorlu   OK

       Gaz, kendisine tanımlanan roleyi açıp kapatan bir objedir. 
       olası tanımlama:
       {
            "name":"gas",
            "uname":"Gaz",
            "id": 5,
            "loc": 1,
            "timer" :0,
            "global":000000XX,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 5, "pcf":1, "name":"ON","type":"PORT_OUTPORT"},
                    {"pin": 6, "pcf":1, "name":"OFF","type":"PORT_OUTPORT"}
                ]
            }
        }

            Objeye, sistem Motorlu ise isimleri ON ve OFF olan en az 2,
                    sistem motorsuz ise 1 çıkış portu tanımlamak zorunludur. 
            
               gas objesi motorlu ve onoff olmak üzere 2 tiptir. Cıkış tipinin
            ne tip olduğu ve oda rölesine baglı çalışıp çalışmayacagı global tanımında 
            bir olarak belirtilir. Global tanımı:
                     0000 00XX şeklindedir. 
                     -------||__ 0 Motorsuz, 1 Motorlu
                        |   |___ 0 Normal, 1 Oda rölesine bağlı
                        |_______ 6 bit, motorlu ise kapatma süresi. (0 ise 15sn alınır)

            Çıkış tipi motorlu ise motorun açılma süresi timer kısmında tanımlanır.
            Değer 0 verilirse default 15sn alınır.                 

            gas oda rolesine baglı olarak tanımlanmış ise Odanın kapanması ile 
        kapatılır, odaya fiziksel olarak girilmesi ile açılır. 
            sistemin yazılım ile açılıp kapatılması mümkündür.

            Yangın veya deprem durumunda gaz kapatılır.

*/