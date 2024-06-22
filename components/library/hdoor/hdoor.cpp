
#include "hdoor.h"
#include "esp_log.h"

static const char *ASANSOR_TAG = "HOME_DOOR";

//Bu fonksiyon inporttan tetiklenir
void HDoor::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        HDoor *door = (HDoor *) arg;
        home_status_t stat = door->get_status();
        bool change = false;
			if (door->genel.active)
			{
				if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
				{
                    if (strcmp(prt->name,"OFF")==0) {stat.stat = false;change=true;}
                    if (strcmp(prt->name,"ON")==0) {stat.stat = true;change=true;}									
				}
			}
			if (change) {
				door->local_set_status(stat,true);
                door->status_change(true);
                door->tim_start();
                ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)door, sizeof(HDoor), portMAX_DELAY));
			}
     }
}


void HDoor::status_change(bool st)
{
  Base_Port *target = port_head_handle;
  while (target) {
      if (target->type == PORT_OUTPORT && status.stat && strcmp(target->name,"ON")==0) 
          {
              target->set_status(st);
          }
      if (target->type == PORT_OUTPORT && !status.stat && strcmp(target->name,"OFF")==0) 
          {
              target->set_status(st);
          }    
      target = target->next;
  }
  if (st) status.status=1; else status.status=0;
  write_status();
  ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(HDoor), portMAX_DELAY));
}

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void HDoor::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
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
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(HDoor), portMAX_DELAY));
        }      
    }
}

void HDoor::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
}

void HDoor::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

void HDoor::tim_callback(void* arg)
{   
    HDoor *aa = (HDoor *)arg;
    aa->status_change(false);
}

void HDoor::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void HDoor::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}

void HDoor::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    HDoor *con = (HDoor *) handler_args;
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

//Alarm mesajları
void HDoor::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    HDoor *con = (HDoor *) handler_args;
    if (st->stat) {
        //alarm deprem mesajı alındı 
        home_status_t ss = con->get_status();
        ss.stat = true;
        con->set_status(ss);
    }
}

void HDoor::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    HDoor *obj = (HDoor *) handler_args;
    Base_Port *target = obj->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL && obj->genel.active)
            {                
                home_status_t ss = obj->get_status();
                ss.stat = st->stat;
                obj->set_status(ss);
            }
        target=target->next;
    }
}

void HDoor::init(void)
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
       HDoor objesi kendine tanımlanan ON, OFF rölelrini belirlenen süre kadar çekip bırakmak üzere düzenlenmiş
       bir objedir. Olası tanım:

       {
            "name":"hdoor",
            "uname":"Kapı",
            "id": 9,
            "loc": 0,
            "timer": 3,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 9, "pcf":1, "name":"ON","type":"PORT_OUTPORT"}
                    {"pin": 10, "pcf":1, "name":"OFF","type":"PORT_OUTPORT"}
                    {"pin": 7, "pcf":1, "name":"ON","type":"PORT_INPORT"}
                    {"pin": 8, "pcf":1, "name":"OFF","type":"PORT_INPORT"}
                    {"pin": 0, "pcf":0, "name":"AN007","type":"PORT_VIRTUAL"}
                ]
            }
        }

        Asansör deprem ve yangın anında kapı otomatik açılacaktır.
        Rölenin basılı kalma süresi timer ile belirlenir.

*/