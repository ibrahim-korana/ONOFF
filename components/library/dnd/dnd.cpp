#include "dnd.h"
#include "esp_log.h"

static const char *DND_TAG = "DND";

#define BIT_DND 0
#define BIT_CLN 1
#define BIT_MIN 2
#define BIT_TEK 3
#define BIT_BUSY 4
#define BIT_FLASH 5
#define BIT_DND_FLASH 6
#define BIT_CLN_FLASH 7

uint8_t Dnd::bit_set(uint8_t number, uint8_t n) {
    return number | ((uint8_t)1 << n);
}
uint8_t Dnd::bit_clear(uint8_t number, uint8_t n) {
    return number & ~((uint8_t)1 << n);
}
uint8_t Dnd::bit_get(uint8_t number, uint8_t n) {
    return (number >> n) & (uint8_t)1;
}

void Dnd::dnd_timer_stop(void)
{
    if (dnd_timer!=NULL)
      if (esp_timer_is_active(dnd_timer)) esp_timer_stop(dnd_timer);
}
void Dnd::dnd_timer_start(void)
{
    dnd_timer_stop();
   if (dnd_timer!=NULL) {
      ESP_ERROR_CHECK(esp_timer_start_once(dnd_timer, dnd_duration * ((1000000 * 60)*10)));
      //ESP_ERROR_CHECK(esp_timer_start_once(dnd_timer, dnd_duration * 1000000));
    } 
}
void Dnd::clean_timer_stop(void)
{
    if (clean_timer!=NULL)
      if (esp_timer_is_active(clean_timer)) esp_timer_stop(clean_timer);
}
void Dnd::clean_timer_start(void)
{
    clean_timer_stop();
    if (clean_timer!=NULL) {
      ESP_ERROR_CHECK(esp_timer_start_once(clean_timer, clean_duration * 1000000));
    }  
}

void Dnd::dnd_timer_callback(void* arg)
{
    Dnd *obj = (Dnd*)arg;
    obj->ledler = obj->bit_set(obj->ledler,BIT_DND_FLASH); 
    ESP_LOGI(DND_TAG,"DND ALARM");
    obj->output_rate();
}  
void Dnd::clean_timer_callback(void* arg)
{
    Dnd *obj = (Dnd*)arg;
    ESP_LOGI(DND_TAG,"CLEAN ALARM");
    obj->ledler = obj->bit_set(obj->ledler,BIT_CLN_FLASH); 
    obj->output_rate();
} 

//Bu fonksiyon inporttan tetiklenir
void Dnd::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port; //hangi buton action üretti
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn); //butonun portu ne
        button_event_t evt = iot_button_get_event(btn); //action eventi ne
        Dnd *obj = (Dnd *) arg; //ben kimim
        //home_status_t stat = obj->get_status(); //Benim statusum ne?
        bool change = false;
        //obje iptal degilse ve event up veya down ise
        if ((evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP) && obj->genel.active)
           {
              if (strcmp(prt->name,"AN_DND")==0)
              {
                if (prt->type==PORT_INPORT)
                {
                   if (evt==BUTTON_PRESS_DOWN) {
                   // printf("dnd basıldı\n");
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_DND); //Dnd bitini 1 yap
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_CLN); //Clean bitini 0 yap
                    change = true;
                   }
                   if (evt==BUTTON_PRESS_UP) {
                   // printf("dnd bırakıldı\n");
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_DND); //Dnd bitini 0 yap
                    change = true;
                   }
                }
                if (prt->type==PORT_INPULS)
                {
                    if (obj->bit_get(obj->anahtarlar,BIT_DND)==0) {
                        obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_DND); //Dnd bitini 1 yap
                        obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_CLN); //Clean bitini 0 yap
                        change = true;
                    } else {
                        obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_DND); //Dnd bitini 0 yap
                        change = true;
                    }
                }
              }

              if (strcmp(prt->name,"AN_CLN")==0)
              {
                if (prt->type==PORT_INPORT)
                {
                   if (evt==BUTTON_PRESS_DOWN) {
                    //printf("cln buton basıldı\n");

                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_CLN); 
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_DND); 
                    change = true;
                   }
                   if (evt==BUTTON_PRESS_UP) {
                    
                     // printf("cln buton bırakıldı\n");

                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_CLN); 
                    change = true;
                   }
                }
                if (prt->type==PORT_INPULS)
                {
                    if (obj->bit_get(obj->anahtarlar,BIT_CLN)==0) {
                        obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_CLN); //Dnd bitini 1 yap
                        obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_DND); //Clean bitini 0 yap
                        change = true;
                    } else {
                        obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_CLN); //Dnd bitini 0 yap
                        change = true;
                    }
                }
              }           
              if (strcmp(prt->name,"AN_MIN")==0)
              {
                if (prt->type==PORT_INPORT)
                {
                   if (evt==BUTTON_PRESS_DOWN) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_MIN); 
                    change = true;
                   }
                   if (evt==BUTTON_PRESS_UP) {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_MIN);
                    change = true; 
                   }
                }
                if (prt->type==PORT_INPULS)
                {
                    if (obj->bit_get(obj->anahtarlar,BIT_CLN)==0) {
                        obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_MIN); 
                        change = true;
                    } else {
                        obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_MIN); 
                        change = true;
                    }
                }
              } 
              if (strcmp(prt->name,"AN_TEK")==0)
              {
                if (prt->type==PORT_INPORT)
                {
                   if (evt==BUTTON_PRESS_DOWN) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_TEK); 
                    change = true;
                   }
                   if (evt==BUTTON_PRESS_UP) {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_TEK); 
                    change = true;
                   }
                }
                if (prt->type==PORT_INPULS)
                {
                    if (obj->bit_get(obj->anahtarlar,BIT_CLN)==0) {
                        obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_TEK); 
                        change = true;
                    } else {
                        obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_TEK);
                        change = true; 
                    }
                }
              } 

              /* if (strcmp(prt->name,"AN_COK")==0)
              {
                if (prt->type==PORT_INPORT)
                {
                   if (evt==BUTTON_PRESS_DOWN) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_COK); 
                    change = true;
                   }
                   if (evt==BUTTON_PRESS_UP) {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_COK); 
                    change = true;
                   }
                }
                if (prt->type==PORT_INPULS)
                {
                    if (obj->bit_get(obj->anahtarlar,BIT_CLN)==0) {
                        obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_COK); 
                        change = true;
                    } else {
                        obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_COK);
                        change = true; 
                    }
                }
              }  */
           }
        if (change) {
            obj->input_rate();
        } 
     }
}

/*
    lokal anahtarlar degişkeni degişti. Degerlendirip işlem yap.
*/
void Dnd::input_rate(void)
{
  //  printf("input %02x\n",anahtarlar);
    if (bit_get(anahtarlar,BIT_CLN)==1) {
            ledler = bit_set(ledler,BIT_CLN); 
            clean_timer_start();
            //printf("cln start\n");
        } else {
            ledler = bit_clear(ledler,BIT_CLN);
            ledler = bit_clear(ledler,BIT_CLN_FLASH);
            clean_timer_stop();
            //printf("cln stop\n");
            }
    if (bit_get(anahtarlar,BIT_DND)==1) { 
            ledler = bit_set(ledler,BIT_DND); 
            dnd_timer_start();
        } else {
            ledler = bit_clear(ledler,BIT_DND); 
            ledler = bit_clear(ledler,BIT_DND_FLASH);
            dnd_timer_stop(); 
        }
    if (bit_get(anahtarlar,BIT_MIN)==1) 
     ledler = bit_set(ledler,BIT_MIN); else ledler = bit_clear(ledler,BIT_MIN);
    if (bit_get(anahtarlar,BIT_TEK)==1) 
     ledler = bit_set(ledler,BIT_TEK); else ledler = bit_clear(ledler,BIT_TEK);
    /* if (bit_get(anahtarlar,BIT_COK)==1) 
     ledler = bit_set(ledler,BIT_COK); else ledler = bit_clear(ledler,BIT_COK); */
    output_rate(); 
}

void Dnd::output_rate(void)
{
  //  printf("output %02x\n",ledler);
    home_virtual_t kk = {};

    Base_Port *target = port_head_handle;
					while (target) {
						if (target->type == PORT_OUTPORT)
						{
                            if (strcmp(target->name,"DND")==0)
                               {
                                   if ((ledler&0x01)==0x01) target->set_status(true); else target->set_status(false); 
                               }
                            if (strcmp(target->name,"CLN")==0)
                               {
                                   if ((ledler&0x02)==0x02) target->set_status(true); else target->set_status(false); 
                               }   
                        }
                        target = target->next;
                    }

    kk.stat = 0;
    kk.sender = 255;
    kk.dnd_status = (anahtarlar<<8) | (ledler);

    strcpy((char*)kk.name,"DND");
    ESP_ERROR_CHECK(esp_event_post(VIRTUAL_SEND_EVENTS, VIRTUAL_DATA, (void*)&kk, sizeof(home_virtual_t), portMAX_DELAY));
    status.color = ledler | (anahtarlar<<8);
    status.stat = true;
    write_status();
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Dnd), portMAX_DELAY));
}

void Dnd::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "color", stt.color);
    cJSON_AddNumberToObject(obj, "oid", genel.oid);
}

void Dnd::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

void Dnd::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    { 
        
        status.color = stat.color;
        ledler = (uint8_t)((status.color &0xF0)>>8);
        anahtarlar = (uint8_t)(status.color & 0x0F);

        //printf("set status %ld LED %02X ANAHTAR %02X\n",status.color,ledler,anahtarlar);

        write_status();
        input_rate();

    }
}

bool Dnd::get_port_json(cJSON* obj)
{
    return false;
}

void Dnd::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Dnd *obj = (Dnd *) handler_args;
    Base_Port *target = obj->port_head_handle;
    bool change=false;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0)
            {                
               if (strcmp(target->name,"SN_DND")==0)
               {
                //printf("DND %d %d\n",st->stat,obj->anahtarlar);
                  if (st->stat) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_DND); //Dnd bitini 1 yap
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_CLN); //Clean bitini 0 yap
                    change = true;
                  } else {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_DND); 
                    change = true;
                  }
               } 
               if (strcmp(target->name,"SN_CLN")==0)
               {
                printf("CLN HAND %d %d\n",st->stat,obj->anahtarlar);
                  if (st->stat) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_CLN); 
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_DND); 
                    change = true;
                  } else {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_CLN); 

                    change = true;
                  }
               } 
               if (strcmp(target->name,"SN_MIN")==0)
               {
                  if (st->stat) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_MIN); 
                    change = true;
                  } else {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_MIN); 
                    change = true;
                  }
               } 
               if (strcmp(target->name,"SN_TEK")==0)
               {
                  if (st->stat) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_TEK); 
                    change = true;
                  } else {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_TEK); 
                    change = true;
                  }
               } 
               /* if (strcmp(target->name,"SN_COK")==0)
               {
                  if (st->stat) {
                    obj->anahtarlar = obj->bit_set(obj->anahtarlar,BIT_COK); 
                    change = true;
                  } else {
                    obj->anahtarlar = obj->bit_clear(obj->anahtarlar,BIT_COK); 
                    change = true;
                  }
               } 
                */
            }
        target=target->next;
    }
    if (change) obj->input_rate();
}

void Dnd::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Dnd *obj = (Dnd *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==obj->genel.device_id || 
           id==ROOM_ON || id==ROOM_OFF || id==ROOM_FON || id==CLEANOK_ON || id==CHECK_IN || id==CHECK_OUT
          // || id==EMERGENCY_ON || id==EMERGENCY_OFF
       ) {
        if (dev_id>0) {
            //Status var bunu kullan
            //printf("DND STATUS %d %d\n",dev_id,obj->genel.device_id);
            obj->set_status(*st);     
        } else {
                if (id==ROOM_FON) {
                    obj->ledler = obj->bit_set(obj->ledler,BIT_BUSY); 
                    obj->output_rate();                
                                  }
                if (id==ROOM_OFF) {
                    obj->ledler = obj->bit_clear(obj->ledler,BIT_BUSY);
                    obj->output_rate();
                }  
                if (id==CLEANOK_ON) {
                    obj->ledler = obj->bit_clear(obj->ledler,BIT_CLN);
                    obj->ledler = obj->bit_clear(obj->ledler,BIT_CLN_FLASH);
                    obj->clean_timer_stop();
                    obj->output_rate();
                }  

                if (id==EMERGENCY_ON) {
                    obj->ledler = obj->bit_set(obj->ledler,BIT_FLASH); 
                    obj->output_rate();                
                                  }
                if (id==EMERGENCY_OFF) {
                    obj->ledler = obj->bit_clear(obj->ledler,BIT_FLASH);
                    obj->output_rate();
                }

                if(id==CHECK_IN) {
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_DND);
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_DND_FLASH);
                   obj->dnd_timer_stop();
                   obj->output_rate();
                }
                if(id==CHECK_OUT) {
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_TEK);
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_MIN); 
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_CLN);
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_CLN_FLASH);
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_DND_FLASH);
                   obj->clean_timer_stop(); 
                   obj->ledler = obj->bit_clear(obj->ledler,BIT_DND);
                   obj->dnd_timer_stop();
                   obj->output_rate();
                } 

        }
    } 
}

void Dnd::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
   // Dnd *obj = (Dnd *) handler_args;
    if (st->stat) {
        //
    } else {
        // 
    }
    
}

void Dnd::init(void)
{
    if (!genel.virtual_device)
    {
        
        esp_timer_create_args_t arg = {};
        arg.callback = &dnd_timer_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &dnd_timer));

        esp_timer_create_args_t arg0 = {};
        arg0.callback = &clean_timer_callback;
        arg0.name = "Ltim1";
        arg0.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg0, &clean_timer));

        dnd_duration = duration;
        clean_duration = global;
        if (dnd_duration==0) dnd_duration=20;//dnd_duration=48; //8 saat
        if (clean_duration==0) clean_duration=10; //clean_duration=1; //10dk

       // set_status(status);
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
    }
}


/*
    Dnd objesi BUSY, DND, CLEAN, TEKNIK, MINIBAR hizmetlerini  yazılım veya donanım ile açıp 
    kapatmak için kullanılan bir objedir.

    Obje içindeki port isimleri sabittir. Tanımlama yapılacaksa aşağıdaki isimler kullanılmalıdır.
    DND fiziksel olarak RS485 üzerinden baglı olmalıdır. 
    
    olası Tanım :

    {
            "name":"dnd",
            "uname":"dnd"
            "id": 5,
            "loc": 1,
            "timer" : 30,
            "global" : 20,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 6, "pcf":1, "name":"AN_DND","type":"PORT_INPORT"},
                    {"pin": 6, "pcf":1, "name":"AN_CLN","type":"PORT_INPORT"},
                    {"pin": 6, "pcf":1, "name":"AN_MIN","type":"PORT_INPORT"},
                    {"pin": 6, "pcf":1, "name":"AN_TEK","type":"PORT_INPORT"},

                    {"pin": 0, "pcf":0, "name":"SN_DND","type":"PORT_VIRTUAL"},
                    {"pin": 0, "pcf":0, "name":"SN_CLN","type":"PORT_VIRTUAL"},
                    {"pin": 0, "pcf":0, "name":"SN_MIN","type":"PORT_VIRTUAL"},
                    {"pin": 0, "pcf":0, "name":"SN_TEK","type":"PORT_VIRTUAL"}
                ]
            }
        }

    Açıp kapatma işlemi ana kutu üzerinde veya remote olarak tanımlanabilir. 

    timer DND zamanlamasını,
    global CLEAN Zamanlamasını tanımlar.
    Zamanlamalar 10dk cinsindendir. 6 60dk, 36 6 saati gösterir.

    DND VİRTUAL ise;
        anahtar ve led bilgileri  home_virtual_t içindeki dnd_an ve dnd_led ve stat alanları
   üzerinden bit bazlı gider ve gelir. 

         dnd_an ANAHTAR anlık bilgilerini içerir
                 XXXX XXXX     
                   || ||||_ 1 dnd anahtarına basılı
                   || |||__ 1 clean anahtarına basılı
                   || ||___ 1 Teknik anahtarına basıldı
                   || |____ 1 Minibar anahtarına basıldı
                   ||______
                   |_______ 
                            

         dnd_led komutlarını içerir
                 XXXX XXXX
                    | ||||_ DND ledi yak
                    | |||__ CLEAN Ledi yak
                    | ||___ TEKNİK ledi yak
                    | |____ MINIBAR Ledi yak
                    |______ BUSY
                            FLASH
                            DND FLASH
                            CLEAN FLASH
*/

