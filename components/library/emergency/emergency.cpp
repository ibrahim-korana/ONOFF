
#include "Emergency.h"
#include "esp_log.h"

static const char *EMERGECY_TAG = "EMERGECY";

void Emergency::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port; //hangi buton action üretti
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn); //butonun portu ne
        button_event_t evt = iot_button_get_event(btn); //action eventi ne
        Emergency *obj = (Emergency *) arg; //ben kimim
        home_status_t stat = obj->get_status(); //Benim statusum ne?
        bool change = false;
        //obje iptal degilse ve event up veya down ise        
        if ((evt==BUTTON_PRESS_DOWN) && obj->genel.active && obj->oda_active)
           {               
                     if (prt->type==PORT_INPORT || prt->type==PORT_INPULS) change = true;
                     
           }
        if (change & !stat.stat) {
            printf("EMERGENCY Algılandı\n");
            stat.stat = true;
            obj->local_set_status(stat,true);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, EMERGENCY_ON, (void *)obj, sizeof(Emergency), portMAX_DELAY));
            ESP_LOGI(EMERGECY_TAG,"EMERGENCY START");
            obj->tim_start();
        } 
     }
}


//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Emergency::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        genel.active = status.active;
        if (status.stat) {
                ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, EMERGENCY_ON, (void *)this, sizeof(Emergency), portMAX_DELAY));
                tim_start();
                ESP_LOGI(EMERGECY_TAG,"EMERGENCY START");
                home_message_t mm = {};
                mm.txt = NULL;
                mm.id = 0;
                ESP_ERROR_CHECK(esp_event_post(MESSAGE_EVENTS, MESSAGE_EMERGENCY, &mm, sizeof(mm), portMAX_DELAY));
                vTaskDelay(50/portTICK_PERIOD_MS); 
            } else {
                ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, EMERGENCY_OFF, (void *)this, sizeof(Emergency), portMAX_DELAY));
                tim_stop();
                ESP_LOGI(EMERGECY_TAG,"EMERGENCY STOP");
            }
    } else {
        bool chg = false;
        if (status.stat!=stat.stat) chg=true;
        if (status.active!=stat.active) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(EMERGECY_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Emergency), portMAX_DELAY));
        }      
    }
}

void Emergency::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "oid", genel.oid);
}

void Emergency::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


void Emergency::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Emergency *emg = (Emergency *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==emg->genel.device_id || id==ROOM_ON || id==ROOM_OFF || id==ROOM_FON) {
        if (dev_id>0) {
            //Status var bunu kullan
                 emg->set_status(*st);     
                      }  else {
                            if ((id==ROOM_ON || id== ROOM_FON) && emg->oda_durumu) emg->oda_active = true;
                            if (id==ROOM_OFF && emg->oda_durumu) {
                                emg->oda_active = false;
                                home_status_t ss = emg->get_status();
                                if (ss.stat) {
                                    ss.stat = false;
                                    emg->set_status(ss);
                                }
                            }
                      }             
    }
}

//Alarm mesajları
void Emergency::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
   // home_virtual_t *st = (home_virtual_t *) event_data;
   // Asansor *con = (Asansor *) handler_args;

}

//Sensor mesajları
void Emergency::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Emergency *emg = (Emergency *) handler_args;
    Base_Port *target = emg->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                
                if (st->stat==1 && emg->genel.active && emg->oda_active)
                  {
                      home_status_t ss = emg->get_status();
                      ss.stat = 1;
                      emg->set_status(ss);
                  }
            }
        target=target->next;
    }
}

void Emergency::tim_callback(void* arg)
{   
    Emergency *aa = (Emergency *)arg;
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, EMERGENCY_ON, (void *)aa, sizeof(Emergency), portMAX_DELAY));
    ESP_LOGI(EMERGECY_TAG,"EMERGENCY ACTIVE");
}

void Emergency::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Emergency::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_periodic(qtimer, duration * 1000000));
}

void Emergency::init(void)
{
    if (!genel.virtual_device)
    {
        if (global==0) {oda_durumu=0;oda_active=true;}
        if (global==1) oda_durumu=1;
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=30;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        set_status(status);
    }
}

/*
       BETA TEST OK

       Emergency objesi oda içinde acil yardım durumunu bildirmek üzere düzenlenmiş bir objedir.
       Herhangi bir çıkış portu bulunmaz.  Ancak DND ve lambaları etkiler, aynı zamanda resepsiyonda
       alarma neden olur. virtual veya fiziksel Giriş portu tanımlanabilir.

       Olası tanım:

       {
            "name":"emergency",
            "uname":"ACIL",
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

        global 0 veya 1 tanımlanabilir. 
           0 tanımlı olması dırımında odanın aktif olup olmadığına bakılmaksızın alarm üretilir. Alarmın
             kapatılması ancak yazılım ile mümkün olabilir. 
           1 tanımlı olması durumunda sadece oda aktif iken acil butonu çalışır durumda olacaktır.
             Alarmın oluşması durumunda odanın kapanması alarmın da kapanmasına neden olur. 
        timer emergecny aktif oldugunda durduruluncaya kadar kaç sn bir alarm üretecegini belirler    

*/