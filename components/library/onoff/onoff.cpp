
#include "onoff.h"
#include "esp_log.h"

static const char *ONOFF_TAG = "ONOFF";

//Bu fonksiyon inporttan tetiklenir
void Onoff::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port; //hangi buton action üretti
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn); //butonun portu ne
        button_event_t evt = iot_button_get_event(btn); //action eventi ne
        Base_Function *obj = (Base_Function *) arg; //ben kimim
        home_status_t stat = obj->get_status(); //Benim statusum ne?
        bool change = false;
        //obje iptal degilse ve event up veya down ise
        if ((evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP) && obj->genel.active)
           {
              //portları tarayıp çıkış portlarını bul
              Base_Port *target = obj->port_head_handle;
              while (target) {
                if (target->type == PORT_OUTPORT) 
                  {  
                     if (prt->type==PORT_INPORT) {
                        if (evt==BUTTON_PRESS_DOWN) {stat.stat = target->set_status(true);change = true;} //çıkış portunun stat ını degiştir.
                        if (evt==BUTTON_PRESS_UP) {stat.stat = target->set_status(false);change = true;}
                     }
                     if (prt->type==PORT_INPULS) {
                        if (evt==BUTTON_PRESS_UP) {stat.stat = target->set_toggle();change = true;}
                     }
                  }       
                target = target->next;
                            }
           }
        if (change) {
            ((Onoff *)obj)->local_set_status(stat,true);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)obj, sizeof(Onoff), portMAX_DELAY));
        } 
     }
}


//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Onoff::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        Base_Port *target = port_head_handle;
        if (status.stat && !counting && status.counter>0 ) {
            tim_start(status.counter);
            counting = true;
        }
        if (!status.stat && counting) {
            counting = false;
            status.counter = 0;
            tim_stop();
        }
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(status.stat);
                }
            target = target->next;
        }
        write_status();
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Onoff), portMAX_DELAY));
    } else {
        bool chg = local_set_status(stat,true);
        if (chg)
        {
            ESP_LOGI(ONOFF_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Onoff), portMAX_DELAY));
        }      
    }
}


//statusu json olarak döndürür
void Onoff::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    if (stt.counter!=255) cJSON_AddNumberToObject(obj, "coun", stt.counter);
    cJSON_AddNumberToObject(obj, "color", genel.icon);
}

void Onoff::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

void Onoff::tim_callback(void* arg)
{   
    Onoff *aa = (Onoff *)arg;
    home_status_t ss = aa->get_status();
    if (ss.counter>0) ss.counter--;
    if (ss.counter==0)
    {     
        Base_Port *target = aa->port_head_handle;
        aa->tim_stop();
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(false);
                }
            target = target->next;
        }
        ss.stat = false;
        aa->set_status(ss);
    }
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)aa, sizeof(Onoff), portMAX_DELAY));
}

void Onoff::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Onoff::tim_start(uint8_t cnt){
    if (qtimer!=NULL)
      if (!esp_timer_is_active(qtimer))
         ESP_ERROR_CHECK(esp_timer_start_once(qtimer, cnt * (1000000*60)));
}


//Sensor mesajları
void Onoff::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Onoff *obj = (Onoff *) handler_args;
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

//Yazılım ile gelen eventler
void Onoff::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Onoff *con = (Onoff *) handler_args;
    uint8_t dev_id = 0;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==con->genel.device_id || id==ROOM_ON || id==ROOM_OFF || id==ROOM_FON) {
        if (dev_id>0) {
            //Status var bunu kullan
            con->set_status(*st);     
        } else {
            if (con->room_cont) 
            { 
                if (id==ROOM_ON) con->room_on = true;
                if (id==ROOM_FON) {
                                     con->room_on = true;
                                     //odaya fiziki olarak girildi kontrol et ve çalıştır.
                                     if (con->room_cont)
                                       {
                                            home_status_t ss = con->get_status();
                                            ss.stat = true;
                                            ss.counter = 0;
                                            con->set_status(ss); 
                                       }                                     
                                  }
                if (id==ROOM_OFF) {
                    con->room_on = false;
                    if (con->room_cont)
                        {
                            home_status_t ss = con->get_status();
                            ss.stat = false;
                            ss.counter = 0;
                            con->tim_stop();
                            con->set_status(ss);  
                        }                  
                }  
            }
        }
    } 
}

//Alarm mesajları
void Onoff::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //Alarm mesajı aldığında alarm_kapat a göre kapatılır.
    home_virtual_t *st = (home_virtual_t *) event_data;
    Onoff *con = (Onoff *) handler_args;
    if (con->alarm_kapat)
    {
        if (st->stat) {
            //Kapat
            con->store_set_status();
            home_status_t ss = con->get_status();
            ss.stat = false;
            con->set_status(ss);
        } else {
            //Eski haline getir
            con->store_get_status();
            con->tim_stop();
            home_status_t ss = con->get_status();
            con->set_status(ss); 
        }  
    }
    
}



void Onoff::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if ((global & 0x01) == 0x01) room_cont = true;
        if ((global & 0x02) == 0x02) alarm_kapat = true;
       
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        set_status(status);
    }
}


/*
    Tanımlanan bir röleyi yazılım veya donanım ile açıp kapatmak için kullanılan bir objedir.
    olası Tanım :

    {
            "name":"onoff",
            "uname":"OnOff",
            "id": 5,
            "loc": 1,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 5, "pcf":1, "name":"Röle","type":"PORT_OUTPORT"},
                    {"pin": 6, "pcf":1, "name":"Lokal Anahtar","type":"PORT_INPORT"},
                    {"pin": 0, "pcf":0, "name":"ON09","type":"PORT_VIRTUAL"},
                    {"pin": 0, "pcf":0, "name":"ON10","type":"PORT_VIRTUAL"},
                ]
            }
        }

    Açıp kapatma işlemi ana kutu üzerinde veya remote olarak tanımlanabilir. 

    Global tanımı bitsel olarak yapılır
         XXXX XXXX
                ||_ 1 Odaya baglı çalış
                ||_ 1 Deprem veya yangında kapat

    Global 1 ise kontaktör oda rolesine baglı olarak çalışır. Odanın kapanması ile 
        kapatılır, odaya fiziksel olarak girilmesi ile açılır.
    
    Program üzerinden gecikmeli kapatma konulabilir.

    Diğer objelerden farklı olarak bu objenin program üzerinde oluşturacagı ikon 
    id üzerinden tanımlanabilir. 
        ID  PROGRAM IKONU
        --  -----------------
        0   Anahtar
        1   Priz
        2   Buzdolabı
        3   Çamaşır Makinası
        4   TV

    
*/

