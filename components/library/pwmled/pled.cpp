
#include "esp_log.h"
#include "pled.h"

static const char *PLED_TAG = "PWMLED";

//Bu fonksiyon inporttan tetiklenir
void PwmLed::func_callback(void *arg, port_action_t action)
{   

   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Base_Function *lamba = (Base_Function *) arg;
        home_status_t stat = lamba->get_status();
        bool change = false;

			if (lamba->genel.active)
			{
				if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
				{
					//portları tarayıp çıkış portlarını bul
					Base_Port *target = lamba->port_head_handle;
					while (target) {
						if (target->type == PORT_PWM)
						{
							if (prt->type==PORT_INPORT) {
								if (evt==BUTTON_PRESS_DOWN && lamba->room_on) {
									stat.stat = target->set_status(true);change = true;((PwmLed *)lamba)->tim_stop();
									printf("ACILDI\n");
								}
								if (evt==BUTTON_PRESS_UP && lamba->room_on) {
									stat.stat = target->set_status(false);change = true;((PwmLed *)lamba)->tim_stop();
									printf("KAPANDI\n");
								}
							}
							if (prt->type==PORT_INPULS) {
								if (evt==BUTTON_PRESS_UP && lamba->room_on) {stat.stat = target->set_toggle();change = true;((PwmLed *)lamba)->tim_stop();}
							}
							if (prt->type==PORT_SENSOR) {
								if (evt==BUTTON_PRESS_DOWN)
								{
									if (prt->status==true) {
										//lamba kapalı lambayı açıp timer çalıştır
										stat.stat = target->set_status(true);
										change = true;
										((PwmLed *)lamba)->tim_stop();
										((PwmLed *)lamba)->tim_start();
									}
								}
							}
						}
						target = target->next;
									}
				}
			}
			if (change) {
				stat.counter = 0;
				((PwmLed *)lamba)->xtim_stop();
				((PwmLed *)lamba)->local_set_status(stat,true);
				if (lamba->function_callback!=NULL) lamba->function_callback(lamba, lamba->get_status());
			}
     }
}

void PwmLed::set_color(uint16_t color)
{
	Base_Port *target = port_head_handle;
	while (target) {
		if (target->type == PORT_PWM)
			{
				status.color = target->set_color(color);
			}
		target = target->next;
	}
}

//bu fonksiyon lambayı yazılım ile tetiklemek için kullanılır.
void PwmLed::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        if (!status.stat) {xtim_stop(); status.counter=0;}
        
        if (status.counter>0) {
           if (status.stat) xtim_start();
        }
        
        if (!genel.active) status.stat = false;
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT || target->type == PORT_PWM)
                {
            	    if (status.color>0) status.color = target->set_color(status.color);
                    status.stat = target->set_status(status.stat);
                }
            target = target->next;
        }
        write_status();
        if (function_callback!=NULL) function_callback((void *)this, get_status());
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz 
//durum değişimleri için kullanılır.
void PwmLed::remote_set_status(home_status_t stat, bool callback_call) {
    bool chg = false;
    if (status.stat!=stat.stat) chg=true;
    if (genel.active!=stat.active) chg=true;
    if (status.counter!=stat.counter) chg=true;

    //printf("before active %d stat active %d chg %d\n",genel.active,stat.active, chg);

    if (chg)
      {
         local_set_status(stat,true);
         ESP_LOGI(PLED_TAG,"%d Status Changed",genel.device_id);
         //printf("after active %d\n",genel.active);
         if (callback_call)
          if (function_callback!=NULL) function_callback((void *)this, get_status());
      }      
}

void PwmLed::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    if (stt.counter>0) cJSON_AddNumberToObject(obj, "coun", stt.counter);
}

void PwmLed::get_status_json(cJSON* obj)
{
    return ConvertStatus(status , obj);
}

/*
void PwmLed::room_event(room_event_t ev)
{
	switch (ev)
	{
	case ROOM_ON: {
		room_on = true;
		printf("pled room on\n");
		break;
	              }
	case ROOM_OFF:{
		room_on = false;
		printf("pled room off\n");
        break;
                  }
	case MAIN_DOOR_ON:break;
	case MAIN_DOOR_OFF:break;
	}
}
*/
//yangın bildirisi aldığında ne yapacak
/*
void PwmLed::fire(bool stat)
{
    xtim_stop();
    status.counter=0;
    if (stat) {
        main_temp_status = status;
        //globali 1 tanımlı lamba acil lambasıdır
        //yangında aktif hale getir.
        //Bu durum lamba start edilmemiş bile olsa geçerlidir.
        //Bunun dışındaki lambalar kapatılır.
        if ((global & 0x01)!=0x01)
        {
           status.stat = false;
        } else {
           status.stat = true; 
        }
        set_status(status);
    } else {
       status = main_temp_status;
       set_status(status);       
    }
}

void PwmLed::senaryo(char *par)
{
    if(strcmp(par,"on")==0)
      {
        status.stat = true; 
        set_status(status);
      }
    if(strcmp(par,"off")==0)
      {
        status.stat = false; 
        set_status(status);
      }  
}
*/
void PwmLed::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void PwmLed::tim_start(void){
    if (qtimer!=NULL)
      ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}
void PwmLed::xtim_stop(void){
    if (xtimer!=NULL)
      if (esp_timer_is_active(xtimer)) esp_timer_stop(xtimer);
}
void PwmLed::xtim_start(void){
    if (xtimer!=NULL)
     if (!esp_timer_is_active(xtimer))
       ESP_ERROR_CHECK(esp_timer_start_periodic(xtimer, 60 * 1000000));
}

void PwmLed::xtim_callback(void* arg)
{   
    //lamba/ları kapat
	PwmLed *aa = (PwmLed *) arg;
    home_status_t stat = aa->get_status();
    bool change=false;
    if (stat.counter>0) {stat.counter--;change=true;}
    if (stat.counter==0)
    {
        aa->xtim_stop();
        Base_Port *target = aa->port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    stat.stat = target->set_status(false);
                    change=true;
                }
            target = target->next;
        }
    }    
    if (change) aa->local_set_status(stat,true);
    if (aa->function_callback!=NULL) aa->function_callback(aa, aa->get_status());
}


void PwmLed::lamp_tim_callback(void* arg)
{   
    //lamba/ları kapat
    Base_Function *aa = (Base_Function *) arg;
    home_status_t stat = aa->get_status();
    Base_Port *target = aa->port_head_handle;
    bool change=false;
    while (target) {
        if (target->type == PORT_OUTPORT) 
            {
                stat.stat = target->set_status(false);
                change=true;
            }
        target = target->next;
    }
    if (change) aa->local_set_status(stat,true);
    if (aa->function_callback!=NULL) aa->function_callback(aa, aa->get_status());
}

void PwmLed::init(void)
{
    if (!genel.virtual_device)
    { 
        esp_timer_create_args_t arg1 = {};
        arg1.callback = &xtim_callback;
        arg1.name = "Ltim1";
        arg1.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg1, &xtimer)); 
        disk.read_status(&status,genel.device_id);
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type==PORT_OUTPORT || target->type==PORT_PWM)
                {
                status.stat = target->get_hardware_status();
                break;
                }
            target=target->next;
        }

        if ((global & 0x02) == 0x02) 
        {
            //Karşılama lambasıdır Timerı tanımla boşta beklesin 
            esp_timer_create_args_t arg = {};
            arg.callback = &lamp_tim_callback;
            arg.name = "Ltim0";
            arg.arg = (void *) this;
            ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
            if (duration==0) duration=30;
        }
    }
}

