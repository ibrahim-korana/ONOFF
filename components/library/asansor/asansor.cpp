
#include "asansor.h"
#include "esp_log.h"

static const char *ASANSOR_TAG = "ASANSOR";

void Asansor::func_callback(void *arg, port_action_t action)
{
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Asansor *asansor = (Asansor *) arg;
        //home_status_t stat = asansor->get_status();
        bool change = false;
			if (asansor->genel.active)
			{
				if (evt==BUTTON_PRESS_DOWN)
				{
					//portları tarayıp çıkış portlarını bul
					Base_Port *target = asansor->port_head_handle;
					while (target) {
						if (target->type == PORT_OUTPORT)
						{
							if (prt->type==PORT_INPORT || prt->type==PORT_INPULS) {
								if (evt==BUTTON_PRESS_DOWN && !asansor->asansor_alarm) {change = true;}
								
							}
						}
						target = target->next;
									}
				}
			}
			if (change) {
				asansor->status_change(true);
                asansor->tim_start();
			}
     }
}


void Asansor::status_change(bool st)
{
  if (genel.active)
  {
    Base_Port *target = port_head_handle;
    while (target) {
        if (target->type == PORT_OUTPORT) 
            {
                target->set_status(st);
            }
        target = target->next;
    }
    if (st) status.status=1; else status.status=0;
    write_status();
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Asansor), portMAX_DELAY));
    //ESP_LOGI(ASANSOR_TAG,"ASANSOR ACTION");
  }
}

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Asansor::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        stat.stat = true;
        local_set_status(stat);
        status_change(true);
        tim_start();
    } else {
        bool chg = false;
        if (status.stat!=stat.stat) chg=true;
        if (status.active!=stat.active) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(ASANSOR_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Asansor), portMAX_DELAY));
        }      
    }
}

void Asansor::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
}

void Asansor::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

void Asansor::tim_callback(void* arg)
{   
    Asansor *aa = (Asansor *)arg;
    aa->status_change(false);
}

void Asansor::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Asansor::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}

void Asansor::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Asansor *con = (Asansor *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==con->genel.device_id) {
        if (dev_id>0) {
            //Status var bunu kullan
            if(!con->asansor_alarm) con->set_status(*st);     
                      } 
    } 
}

//Alarm mesajları
void Asansor::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Asansor *con = (Asansor *) handler_args;
    if (st->stat) {
        //alarm deprem mesajı alındı 
        con->asansor_alarm = true;
    } else {
        con->asansor_alarm = false;
    }  
}

void Asansor::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Asansor *asansor = (Asansor *) handler_args;
    Base_Port *target = asansor->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0)
            {                
                home_status_t ss = asansor->get_status();
                if (asansor->genel.active && !asansor->asansor_alarm) 
                  {                     
                     if (strncmp(target->name,"AN",2)==0)
                       {
                          //Bu bir anahtardır
                          asansor->tim_stop();
                          ss.stat = st->stat;
                          asansor->set_status(ss);
                       }  
                  }
            }
        target=target->next;
    }
}


void Asansor::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=3;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
    }
}



/*
       Asansor objesi kendine tanımlanan röleyi belirlenen süre kadar çekip bırakmak üzere düzenlenmiş
       bir objedir. Yazılım/Donanım ile tetiklenebilir. Olası tanım:

       {
            "name":"elev",
            "uname":"asansor",
            "id": 9,
            "loc": 0,
            "timer": 3,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 9, "pcf":1, "name":"role","type":"PORT_OUTPORT"}
                    {"pin": 7, "pcf":1, "name":"ANAHTAR","type":"PORT_INPORT"}
                    {"pin": 0, "pcf":0, "name":"AN007","type":"PORT_VIRTUAL"}
                ]
            }
        }

        Asansör deprem ve yangın anında çalışmayacaktır.

        timer asansor rolesinin basılı kalma suresidir. 0 tanımlanırsa 3sn alınır.


*/