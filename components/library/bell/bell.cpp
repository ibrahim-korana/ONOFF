
#include "bell.h"
#include "esp_log.h"

static const char *BELL_TAG = "BELL";

void Bell::func_callback(void *arg, port_action_t action)
{
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Bell *bell = (Bell *) arg;
        //home_status_t stat = bell->get_status();
        bool change = false;
			if (bell->genel.active)
			{
				if (evt==BUTTON_PRESS_DOWN)
				{
                    if (prt->type==PORT_INPORT || prt->type==PORT_INPULS) {
                        if (evt==BUTTON_PRESS_DOWN && !bell->bell_alarm) {change = true;}
                        
                    }
				}
			}
			if (change) {
				bell->status_change(true);
                bell->tim_start();
			}
     }
}

void Bell::status_change(bool st)
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
  ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Bell), portMAX_DELAY));
}

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Bell::set_status(home_status_t stat)
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
            ESP_LOGI(BELL_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Bell), portMAX_DELAY));
        }      
    }
}

void Bell::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
}

void Bell::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

void Bell::tim_callback(void* arg)
{   
    Bell *aa = (Bell *)arg;
    aa->status_change(false);
}

void Bell::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Bell::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}

void Bell::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Bell *con = (Bell *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==con->genel.device_id) {
        if (dev_id>0) {
            //Status var bunu kullan
            if(!con->bell_alarm)
                 con->set_status(*st);     
                      }               
    } else {
        if (id==ROOM_FON) {
             con->room_on = true;
        }
        if (id==ROOM_OFF) {
             con->room_on = false; 
        }
        if (id==DND_ON) {
             con->bell_alarm=true;
            // ESP_LOGI(BELL_TAG,"BELL Closed");
        }
        if (id==DND_OFF) {
            con->bell_alarm=false;
           // ESP_LOGI(BELL_TAG,"BELL Opened");
        }
        if (id==FIRE_ON &&  con->room_on)
        {
            if (con->global==1 && ++con->fire_count>FIRE_DELAY-1)
              {
                 home_status_t ss = con->get_status();
                 ss.stat = true;
                 con->set_status(ss);  
              }
        }
        if (id==FIRE_OFF)
        {
            con->fire_count = 0;
        }
    }
}

//Alarm mesajları
void Bell::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
   // home_virtual_t *st = (home_virtual_t *) event_data;
   // Asansor *con = (Asansor *) handler_args;

}

//Sensor mesajları
void Bell::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Bell *klm = (Bell *) handler_args;
    Base_Port *target = klm->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                
                if (st->stat==1 && klm->genel.active && !klm->bell_alarm)
                  {
                      home_status_t ss = klm->get_status();
                      ss.stat = 1;
                      klm->set_status(ss);
                  }
            }
        target=target->next;
    }
}

void Bell::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=1;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
    }
}

/*
       Bell objesi kendine tanımlanan röleyi belirlenen süre kadar çekip bırakmak üzere düzenlenmiş
       bir objedir. Sadece yazılım ve virtual anahtarlar ile tetiklenebilir. Olası tanım:

       {
            "name":"bell",
            "uname":"ZIL",
            "id": 9,
            "loc": 0,
            "timer": 3,
            "global": 1,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 10, "pcf":1, "name":"role","type":"PORT_OUTPORT"},
                    {"pin": 0, "pcf":0, "name":"AN03","type":"PORT_VIRTUAL"},
                ]
            }
        }

        ZIL DND aktif ise çalışmayacaktır.
        global 1 tanımlı ise yangın mesajı alındığında odada biri varsa gecikmeli olarak zil çalacaktır.

*/