
#include "lampon.h"
#include "esp_log.h"

static const char *LAMPON_TAG = "LAMPON";

void Lampon::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port; //hangi buton action üretti
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn); //butonun portu ne
        button_event_t evt = iot_button_get_event(btn); //action eventi ne
        Lampon *obj = (Lampon *) arg; //ben kimim
        home_status_t stat = obj->get_status(); //Benim statusum ne?
        bool change = false;
        //obje iptal degilse ve event up veya down ise        
        if ((evt==BUTTON_PRESS_DOWN) && obj->genel.active && obj->room_on)
           {               
                     if (prt->type==PORT_INPORT || prt->type==PORT_INPULS) change = true;
                     
           }
        if (change) obj->set_status(stat);
     }
}


//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Lampon::set_status(home_status_t stat)
{      
    if (status.status==0)
    {
        status.stat=true;
        status.status=1;
        write_status();
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, LAMP_ALL_ON, (void *)this, sizeof(Lampon), portMAX_DELAY));
        ESP_LOGI(LAMPON_TAG,"Full Lamp ON");
        tim_stop();
        tim_start();
    } else {
        status.stat=true;
        status.status=0;
        write_status();
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, LAMP_ALL_OFF, (void *)this, sizeof(Lampon), portMAX_DELAY));
        ESP_LOGI(LAMPON_TAG,"Full Lamp OFF");
        tim_stop();
        tim_start();
    }
}


//statusu json olarak döndürür
void Lampon::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj,"status",stt.status);
}

void Lampon::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


//Yazılım ile gelen eventler
void Lampon::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Lampon *con = (Lampon *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==con->genel.device_id && dev_id>0) 
    {
            //Status var bunu kullan
            con->set_status(*st);     
    } else {
        if (id==ROOM_FON) con->room_on = true;
        if (id==ROOM_OFF) con->room_on = false;    
    }
}

void Lampon::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Lampon *emg = (Lampon *) handler_args;
    Base_Port *target = emg->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                
                if (emg->genel.active && emg->room_on)
                  {
                      home_status_t ss = emg->get_status();
                      emg->set_status(ss);
                  }
            }
        target=target->next;
    }
}

void Lampon::tim_callback(void* arg)
{   
    Lampon *aa = (Lampon *)arg;
    home_status_t ss = aa->get_status();
    ss.stat = false;
    aa->local_set_status(ss);
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)aa, sizeof(Lampon), portMAX_DELAY));
}

void Lampon::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Lampon::tim_start(void){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}


void Lampon::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=2;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
       // set_status(status);
    }
}


/*
    Tüm lambaları kapatıp açmak için  kullanılan bir objedir.
    olası Tanım :

        {
            "name":"lampon",
            "uname":"Lamp Control",
            "id": 5,
            "loc": 1,
            "timer": 2,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 10, "pcf":1, "name":"acil1","type":"PORT_INPORT"},
                    {"pin": 0, "pcf":0, "name":"AN09","type":"PORT_VIRTUAL"}
                ]
            }
        }

    Obje LAMP_ALL_ON ve LAMP_ALL_OFF mesajlarını yayınlar. 
    Virtual veya fiziksel giriş portları tanımlanabilir. Bir çıkış portu yoktur.
    Obje ON veya OFF olarak aktive edildiğinde timer ile belirlenen süresin sonunda 
    off (stat=false) durumuna geçer. Lambaları ON/OFF yapacagı status üzerinden takip edilebilir. 


    

  
*/

