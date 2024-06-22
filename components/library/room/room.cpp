

#include "room.h"
#include "esp_log.h"

static const char *ROOM_TAG = "ROOM";

ESP_EVENT_DEFINE_BASE(FUNCTION_OUT_EVENTS);
ESP_EVENT_DEFINE_BASE(FUNCTION_IN_EVENTS);


void Room::func_callback(void *arg, port_action_t action)
{
//printf("GELDI \n");
   if (action.action_type==ACTION_INPUT)
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Room *oda = (Room *) arg;
        //home_status_t stat = oda->get_status();
        //bool change = false;

        	if ((evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP) && oda->genel.active)
			{
        		if (prt->type==PORT_INPORT)
				{
        			//EQU
					if (evt==BUTTON_PRESS_UP) {
						//Kart Çıkartıldı
                        //printf("KART OFF\n");
						oda->ytim_start();
					}
					if (evt==BUTTON_PRESS_DOWN) {
						//Kart Takıldı
                        //printf("KART ON\n");
						if (oda->ytim_is_active()) oda->ytim_stop();
                        home_status_t st = oda->get_status();
                        if (!st.stat) {
                            st.stat = true;
                            st.status = 1;
                            oda->set_status(st);
                        }
					}
				}        		
		} //evt
     } //ACTION_INPUT
}


//bu fonksiyon odayı yazılım ile tetiklemek için kullanılır.
void Room::set_status(home_status_t stat)
{
    if (!genel.virtual_device)
    {
        local_set_status(stat);
        if (status.active!=genel.active) status.active = genel.active; 
        if (!genel.active) status.stat = false;

        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT)
                {
                    status.stat = target->set_status(status.stat);
                }
            target = target->next;
        }
        write_status();
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Room), portMAX_DELAY));
        if (status.stat) {
                            if (status.status==0) 
                               ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ON, (void *)this, sizeof(Room), portMAX_DELAY)); 
                            else    
                               ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_FON, (void *)this, sizeof(Room), portMAX_DELAY)); 
                         } else ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_OFF, (void *)this, sizeof(Room), portMAX_DELAY));
       // if (!status.active) else ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_DISABLE, (void *)this, sizeof(Room), portMAX_DELAY));                
    } else {
        if (command_callback!=NULL) command_callback((void *)this, stat);
    }
}

//Eger mevcut durumdan fark var ise statusu ayarlar ve/veya callback çağrılır
//durum degişimi portları etkilemez. bu fonksiyon daha çok remote cihaz
//durum değişimleri için kullanılır.
void Room::remote_set_status(home_status_t stat, bool callback_call)
{
	//
	ESP_LOGI(ROOM_TAG,"%d Status Changed",genel.device_id);
}

void Room::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "status", stt.status);
    cJSON_AddNumberToObject(obj, "oid", genel.oid);
}

void Room::get_status_json(cJSON* obj)
{
    return ConvertStatus(status , obj);
}


void Room::ytim_stop(void){
    if (ytimer!=NULL)
      if (esp_timer_is_active(ytimer)) {
    	  esp_timer_stop(ytimer);
          ESP_LOGI(ROOM_TAG,"Kapatma Iptal Edildi");
      }

}
void Room::ytim_start(void){
    if (ytimer!=NULL)
     if (!esp_timer_is_active(ytimer))
     {
        ESP_ERROR_CHECK(esp_timer_start_once(ytimer, duration * 1000000));
        ESP_LOGI(ROOM_TAG,"Oda Kapatiliyor..");
     }
}

void Room::xtim_callback(void* arg)
{
    //lamba/ları kapat
    Room *oda = (Room *) arg;
    home_status_t stat = oda->get_status();
    stat.stat=false;
    stat.status = 0;
    oda->set_status(stat);
    ESP_LOGI(ROOM_TAG,"Room OFF");
  //  ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_OFF, (void *)oda, sizeof(Room), portMAX_DELAY));
}


/*
    FUNCTION_IN_EVENT mesajlarını dinler. Gelen mesaj parametresinde kendi id si varsa veya 
    mesaj ROOM_EXT_ON veya OFF ise odayı kapatır veya açar. 
*/
void Room::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Room *room = (Room *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id; 
    if (dev_id==room->genel.device_id || id==ROOM_EXT_ON || id==ROOM_EXT_OFF) {
        if (dev_id>0) {
            //Status var bunu kullan
            room->set_status(*st);     
        } else {
                home_status_t ss = room->get_status();   
                if (id==ROOM_EXT_ON) ss.stat = true;
                if (id==ROOM_EXT_OFF) ss.stat = false;
                room->set_status(ss);                
        }
    } 
}

void Room::init(void)
{
    if (!genel.virtual_device)
    {
        //yTimer ı oluştur 
        esp_timer_create_args_t arg2 = {};
        arg2.callback = &xtim_callback;
        arg2.name = "Ltim2";
        arg2.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg2, &ytimer));
        
        if (duration==0) duration=5;
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        //Inport durumunu oku
        Base_Port *target = port_head_handle;
        bool indrm = false;
        while (target) {
            if (target->type==PORT_INPORT)
                {
                   indrm = target->get_hardware_status();
                   break;
                }
            target=target->next;
        }

        if (indrm) status.stat = true; 
        set_status(status);
    }
}

/*
      Room Test OK
          iptal Test OK
          Remote On/Off test OK

      Oda objesi otel odasını yönetmek için düzenlenmiştir. Olası tanım:
      {
            "name":"room",
            "uname":"room",
            "icon": 0,
            "loc": 0,     
            "timer": 5,
            "global":0,
            "hardware": {
                "port": [
                    {"pin": 1, "pcf":1, "name":"equ", "type":"PORT_INPORT"},
                    {"pin": 12, "pcf":1, "name":"oda role", "type":"PORT_OUTPORT"}
                ]
            }
        },

*/
