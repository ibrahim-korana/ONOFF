
#include "clnok.h"
#include "esp_log.h"

static const char *CLNOK_TAG = "CLNOK";

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void ClnOK::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);      
        write_status();
        if (stat.stat)
        {
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, CLEANOK_ON, (void *)this, sizeof(ClnOK), portMAX_DELAY));
            home_message_t mm = {};
            mm.txt = NULL;
            mm.id = 0;
            ESP_ERROR_CHECK(esp_event_post(MESSAGE_EVENTS, MESSAGE_CLNOK, &mm, sizeof(mm), portMAX_DELAY));
            vTaskDelay(50/portTICK_PERIOD_MS); 
            tim_stop();
            tim_start();
        } 
        else {
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(ClnOK), portMAX_DELAY));
            tim_stop();
        }
    } else {
        bool chg = local_set_status(stat,true);
        if (chg)
        {
            ESP_LOGI(CLNOK_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(ClnOK), portMAX_DELAY));
        }      
    }
}


//statusu json olarak döndürür
void ClnOK::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    cJSON_AddTrueToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "oid", genel.oid);
}

void ClnOK::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

//Sensor mesajları
void ClnOK::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    ClnOK *obj = (ClnOK *) handler_args;
    Base_Port *target = obj->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                
                home_status_t ss = obj->get_status();
                ss.stat = st->stat;
                obj->set_status(ss);
            }
        target=target->next;
    }
}

//Yazılım ile gelen eventler
void ClnOK::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    ClnOK *con = (ClnOK *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==con->genel.device_id || id==CLEAN_ON) {
        if (dev_id>0) {
            //Status var bunu kullan
            con->set_status(*st);     
        } else {
            if (id==CLEAN_ON) {
                home_status_t ss = con->get_status();
                if (ss.stat) {
                    ss.stat = false;
                    con->set_status(ss); 
                }               
                              }
        }
    } 
}

void ClnOK::tim_callback(void* arg)
{   
    ClnOK *aa = (ClnOK *)arg;
    //clnok kapatılacak
    home_status_t st = aa->get_status();
    st.stat = false;
    aa->set_status(st); 
}

void ClnOK::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void ClnOK::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}

void ClnOK::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=30;

        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        set_status(status);
    }
}


/*
    Temizliğin tamamlandıgını takip için  kullanılan bir objedir.
    olası Tanım :

    {
            "name":"clnok",
            "uname":"Clean OK",
            "id": 5,
            "loc": 1,
            "timer":10,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 0, "pcf":0, "name":"ON10","type":"PORT_VIRTUAL"}
                ]
            }
        }

    Açıp kapatma işlemi remote olarak tanımlanabilir. 
    Clean ile birlikte çalışır. Clean açıldığında cleanok Kapatılır.,
    Cleanok Açıldığında ise clean kapatılır. 
    cleanok açık kalma süresi timerda tanımlanır dk cinsinden
  
*/

