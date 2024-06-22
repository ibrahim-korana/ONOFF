

#include "curtain.h"
#include "esp_log.h"

static const char *CUR_TAG = "CURTAIN";

//Bu fonksiyon inporttan tetiklenir
void Curtain::func_callback(void *arg, port_action_t action)
{   
    Curtain *fnc = (Curtain *) arg;
    if (fnc->genel.active)
    {
        if (action.action_type==ACTION_INPUT) 
        {
            button_handle_t *btn = (button_handle_t *) action.port;
            Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
            button_event_t evt = iot_button_get_event(btn);       
            home_status_t stat = fnc->get_status();
            bool change = false;

            //printf("Event %d\n",evt);

            if (evt==BUTTON_LONG_PRESS_START) 
            {
                fnc->buton_durumu = CURBTN_LONG;
            }
            if (evt==BUTTON_PRESS_DOWN)
            {
                if (fnc->Motor!=MOTOR_STOP)
                    {
                    //motor calışıyor. motoru ve timerı durdur
                    fnc->Motor=MOTOR_STOP;
                    fnc->tim_stop();
                    stat.status = (int)CUR_HALF_CLOSED;
                    change=true;
                    fnc->temp_status = CUR_UNKNOWN;
                    fnc->motor_action();
                    } else {
                    //motor stop durumda. motoru çalıştırıp zamanı başlat
                    fnc->temp_status = (cur_status_t) fnc->status.status;
                    if (strcmp(prt->name,"UP")==0) fnc->Motor=MOTOR_UP;
                    if (strcmp(prt->name,"DOWN")==0) fnc->Motor=MOTOR_DOWN;
                    stat.status = (int) CUR_PROCESS;
                    change=true;
                    fnc->tim_stop();
                    fnc->tim_start();
                    fnc->motor_action();
                    }
            }
            if (evt==BUTTON_PRESS_UP) 
            {
                if (fnc->buton_durumu==CURBTN_LONG || fnc->eskitip) {
                    fnc->temp_status = CUR_UNKNOWN;
                    fnc->buton_durumu = CURBTN_UNKNOWN;
                    //motoru ve timer ı durdur
                    stat.status = (int) CUR_HALF_CLOSED;
                    change=true;
                    fnc->Motor=MOTOR_STOP;
                    fnc->tim_stop();
                    fnc->motor_action();
                }; 
            }
        if (change) fnc->local_set_status(stat,true);
        }
    }
}

void Curtain::tim_stop(void){
  if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Curtain::tim_start(void){
  if (status.counter==0 || status.counter>60) status.counter=duration;
  ESP_ERROR_CHECK(esp_timer_start_once(qtimer, status.counter * 1700000));
}

const char* Curtain::convert_motor_status(uint8_t mot)
{
  if (mot==MOTOR_STOP) return "STOP";
  if (mot==MOTOR_UP) return "UP";
  if (mot==MOTOR_DOWN) return "DOWN";
  return "UNKNOWN";
}
void Curtain::motor_action(bool callback) 

{  /*
    0 stop
    1 up
    2 down
    */
  Base_Port *target = port_head_handle;
  while (target) { 
    if (Motor==MOTOR_STOP) 
    {
        if (target->type==PORT_OUTPORT) target->set_status(false);
    }
    if (Motor==MOTOR_UP) 
    {
        if (target->type==PORT_OUTPORT && strcmp(target->name,"UP")==0) target->set_status(true);
    }
    if (Motor==MOTOR_DOWN) 
    {
        if (target->type==PORT_OUTPORT && strcmp(target->name,"DOWN")==0) target->set_status(true);
    }  
    target = target->next;
  }

  ESP_LOGI(CUR_TAG,"Motor Status : %s",convert_motor_status(Motor));
  ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Curtain), portMAX_DELAY));
}

void Curtain::cur_tim_callback(void* arg)
{
    Curtain *mthis = (Curtain *)arg;
    home_status_t st = mthis->get_status();
    ESP_LOGI(CUR_TAG,"Bekleme zamanı doldu");
    if (mthis->temp_status!=CUR_UNKNOWN)
    {   
        ESP_LOGI(CUR_TAG,"MOTOR Durduruluyor..");
        st.status = (int) mthis->temp_status;
        mthis->local_set_status(st,true);   
        mthis->temp_status = CUR_UNKNOWN;
        mthis->Motor= MOTOR_STOP;
        mthis->motor_action(); 
    }
}


void Curtain::set_status(home_status_t stat)
{
  if (!genel.virtual_device)
    {
    if (stat.active!=genel.active) genel.active = stat.active; 
    local_set_status(stat,true);
    if (genel.active)
        {
            if (temp_status == CUR_UNKNOWN) 
            {
                if (status.status==0 || status.status==2)
                    {     tim_stop();
                        tim_start();
                        temp_status = (cur_status_t) status.status;
                        status.status = (int) CUR_PROCESS;
                        if (temp_status == CUR_CLOSED) Motor = MOTOR_DOWN;
                        if (temp_status == CUR_OPENED) Motor = MOTOR_UP;
                        motor_action(false);
                    }
            } else {
                status.status = (int)CUR_HALF_CLOSED;
                tim_stop();
                temp_status = CUR_UNKNOWN;
                Motor= MOTOR_STOP;
                motor_action(false);
            }
            write_status();
        }
    } else {
        bool chg = false;
        if (status.status!=stat.status) chg=true;
        if (status.active!=genel.active) chg=true;
        if (status.counter!=stat.counter) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(CUR_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Curtain), portMAX_DELAY));  
        }   
    }
}

void Curtain::ConvertStatus(home_status_t stt, cJSON *obj)
{
    cJSON_AddNumberToObject(obj, "status", stt.status);
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "coun", stt.counter);
}

void Curtain::get_status_json(cJSON* obj)
{
    return ConvertStatus(status, obj);
}


/*
void Curtain::fire(bool stat)
{
    if (stat) {
        main_temp_status = status;
        status.status = CUR_OPENED;
        Motor = MOTOR_UP;
        motor_action();
    } else {
       status = main_temp_status;
       set_status(status);       
    }
}

void Curtain::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.status = CUR_OPENED;
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.status = CUR_CLOSED;
        set_status(status);
      }  
}
*/

void Curtain::role_change(void)
{
  Base_Port *target = port_head_handle;
  while (target) 
  { 
    Base_Port *pp = (Base_Port *) target;
    if (pp->type == PORT_OUTPORT) 
    {
      if (strcmp(pp->name,"UP")==0)
      {
        strcpy(pp->name,"DOWN");
      } else strcpy(pp->name,"UP");
    }
    target = target->next;
  } 
}

void Curtain::anahtar_change(void)
{
  Base_Port *target = port_head_handle;
  while (target) 
  { 
    Base_Port *pp = (Base_Port *) target;
    if (pp->type == PORT_INPORT) 
    {
      if (strcmp(pp->name,"UP")==0)
      {
        strcpy(pp->name,"DOWN");
      } else strcpy(pp->name,"UP");
    }
    target = target->next;
  }  
}


//Alarm mesajları
void Curtain::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Curtain *con = (Curtain *) handler_args;
    if (st->stat) {
        //alarm deprem mesajı alındı gazı kapat
        con->store_set_status();
        home_status_t ss = con->get_status();
        ss.status=CUR_OPENED;
        con->set_status(ss);
    } else {
        con->store_get_status();
        home_status_t ss = con->get_status();
        con->set_status(ss); 
    }  
}

void Curtain::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Curtain *con = (Curtain *) handler_args;
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

//Sensor mesajları
void Curtain::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Curtain *klm = (Curtain *) handler_args;
    Base_Port *target = klm->port_head_handle;
    uint8_t i=1;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                
                home_status_t ss = klm->get_status();
                if (klm->genel.active) 
                  {                     
                    if (strncmp(target->name,"UP",2)==0)
                       { 
                          if (st->stat)
                            { 
                                ss.status = CUR_OPENED;
                                klm->set_status(ss);
                            }
                       }  
                    if (strncmp(target->name,"DO",2)==0)
                       {  
                        if (st->stat)
                            { 
                                ss.status = CUR_CLOSED;
                                klm->set_status(ss);
                            }
                          
                       }     
                  }
            }
        i++;    
        target=target->next;
    }
}


void Curtain::init(void)
{
  if (!genel.virtual_device)
  {
    status.stat = true;
    status.status = CUR_CLOSED;
    esp_timer_create_args_t arg = {};
    arg.callback = &cur_tim_callback;
    arg.name = "tim0";
    arg.arg = (void *) this;
    ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
    if (status.counter==0 || status.counter>60) status.counter=duration;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
    ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
    Motor= MOTOR_STOP;
    motor_action(false);   
  }
}

/*
     Curtain perde açıp kapatmak üzere düzenlenmiştir. olası tanım:

     {
            "name":"cur",
            "uname":"Perde",
            "id": 10,
            "timer": 60,            
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 13,"pcf":1, "name":"UP", "type":"PORT_OUTPORT"},
                    {"pin": 14,"pcf":1, "name":"DOWN", "type":"PORT_OUTPORT"},
                    {"pin": 3,"pcf":1, "name":"UP", "type":"PORT_INPORT"},
                    {"pin": 4,"pcf":1, "name":"DOWN", "type":"PORT_INPORT"}
                ]
            }
    }

    objede isimleri UP ve DOWN olan en az 2 çıkış portu tanımlanmak zorundadır. 
    Eger fiziki anahtar baglanacaksa bu anahtarlarun isimleri de UP ve DOWN 
    olmak zorundadır. 
    Objeye virtual anahtar baglanabilir. Baglanacak bu anahtarların isimleri
    UPxxxxx veya DOWNxxxx şeklinde olmalıdır. 

    perdenin açılma/kapanma süresi timer içinde tanımlanır. 


*/