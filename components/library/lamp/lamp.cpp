
#include "lamp.h"
#include "esp_log.h"

static const char *LAMP_TAG = "LAMP";

ESP_EVENT_DEFINE_BASE(FUNCTION_EVENTS);
ESP_EVENT_DEFINE_BASE(VIRTUAL_EVENTS);
ESP_EVENT_DEFINE_BASE(ALARM_EVENTS);
ESP_EVENT_DEFINE_BASE(FUNCTION_REMOTE_EVENTS);


//Virtual anahtarlar varsa bu anahtarlara lambanın durumunu gönderir
void Lamp::send_virtual_anahtar(uint8_t sender)
{
    Base_Port *target = port_head_handle;
    while (target) {
        if (target->type==PORT_VIRTUAL && strncmp(target->name,"AN",2)==0)
          {
            //Eger port bildirim gelecek olarak işaretlenmiş ise Bu porta bildirim gönder
            if (target->ack) {
                home_status_t pp = get_status();
                home_virtual_t kk = {};
                kk.stat = pp.stat;
                kk.sender = sender;
                strcpy((char*)kk.name,target->name);
                ESP_ERROR_CHECK(esp_event_post(VIRTUAL_SEND_EVENTS, VIRTUAL_DATA, (void*)&kk, sizeof(home_virtual_t), portMAX_DELAY));
                }
          }
        target = target->next;
    } 
}

//Bu fonksiyon inporttan tetiklenir
void Lamp::func_callback(void *arg, port_action_t action)
{   
   if (action.action_type==ACTION_INPUT) 
     {
        button_handle_t *btn = (button_handle_t *) action.port;
        Base_Port *prt = (Base_Port *) iot_button_get_user_data(btn);
        button_event_t evt = iot_button_get_event(btn);
        Lamp *lamba = (Lamp *) arg;
        home_status_t stat = lamba->get_status();
        if (!lamba->room_lamp) lamba->room_on=true; 
        bool change = false;
			if (lamba->genel.active)
			{
				if (evt==BUTTON_PRESS_DOWN || evt==BUTTON_PRESS_UP)
				{
					//portları tarayıp çıkış portlarını bul
					Base_Port *target = lamba->port_head_handle;
					while (target) {
						if (target->type == PORT_OUTPORT)
						{
							if (prt->type==PORT_INPORT) {
                                ((Lamp *)lamba)->tim_stop();
								if (evt==BUTTON_PRESS_DOWN && lamba->room_on) {stat.stat = target->set_status(true);change = true;}
								if (evt==BUTTON_PRESS_UP && lamba->room_on) {stat.stat = target->set_status(false);change = true;}
							}
							if (prt->type==PORT_INPULS) {
								if (evt==BUTTON_PRESS_UP && lamba->room_on) {stat.stat = !target->set_toggle();change = true;((Lamp *)lamba)->tim_stop();}
							}
							if (prt->type==PORT_SENSOR) {
								if (evt==BUTTON_PRESS_DOWN)
								{
									if (prt->status==true) {
										//lamba kapalı lambayı açıp timer çalıştır
										stat.stat = target->set_status(true);
										change = true;
										((Lamp *)lamba)->tim_stop();
										((Lamp *)lamba)->tim_start();
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
				((Lamp *)lamba)->xtim_stop();
				((Lamp *)lamba)->local_set_status(stat,true);
                lamba->send_virtual_anahtar(255);
                ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)lamba, sizeof(Lamp), portMAX_DELAY));
			}
     }
}


//bu fonksiyon lambayı yazılım ile tetiklemek için kullanılır.
void Lamp::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    {   
        local_set_status(stat);
        if (stat.active!=genel.active) genel.active = stat.active;
        if (!genel.active) status.stat = false;

        if (!status.stat) {xtim_stop(); status.counter=0;}
        
        if (status.counter>0) {
           if (status.stat) xtim_start();
        }
                  
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT) 
                {
                    target->set_status(status.stat);
                }
            target = target->next;
        }
        write_status();
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Lamp), portMAX_DELAY));
        send_virtual_anahtar(255);
    } else {
        bool chg = false;
        if (status.stat!=stat.stat) chg=true;
        if (genel.active!=stat.active) chg=true;
        if (status.counter!=stat.counter) chg=true;
        if (chg)
        {
            local_set_status(stat,true);
            ESP_LOGI(LAMP_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Lamp), portMAX_DELAY));
            
        }             
    }
}

void Lamp::set_status_non_message(home_status_t stat)
{      
    local_set_status(stat);          
    Base_Port *target = port_head_handle;
    while (target) {
        if (target->type == PORT_OUTPORT) 
            {
                status.stat = target->set_status(status.stat);
            }
        target = target->next;
    }
}


void Lamp::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    if (stt.counter>0) cJSON_AddNumberToObject(obj, "coun", stt.counter);
}

void Lamp::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

bool Lamp::get_port_json(cJSON* obj)
{
    return false;
}


void Lamp::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Lamp::tim_start(void){
    if (qtimer!=NULL) {
      ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
    }
}
void Lamp::xtim_stop(void){
    if (xtimer!=NULL)
      if (esp_timer_is_active(xtimer)) esp_timer_stop(xtimer);
}
void Lamp::xtim_start(void){
    if (xtimer!=NULL)
     if (!esp_timer_is_active(xtimer))
       ESP_ERROR_CHECK(esp_timer_start_periodic(xtimer, 60 * 1000000));
}

void Lamp::xtim_callback(void* arg)
{   
    //lamba/ları kapat
    Lamp *aa = (Lamp *) arg;
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
    //if (aa->function_callback!=NULL) aa->function_callback(aa, aa->get_status());
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_EVENTS, CALLBACK_DATA, aa, sizeof(Base_Function), portMAX_DELAY));
    aa->send_virtual_anahtar(255);
}


void Lamp::lamp_tim_callback(void* arg)
{   
    //lamba/ları kapat
    Lamp *aa = (Lamp *) arg;
    home_status_t stat = aa->get_status();
    Base_Port *target = aa->port_head_handle;
   // bool change=false;
    while (target) {
        if (target->type == PORT_OUTPORT) target->set_status(false);
        target = target->next;
    }
    stat.stat = false;
    aa->local_set_status(stat,true);
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)aa, sizeof(Base_Function), portMAX_DELAY));
    aa->send_virtual_anahtar(255);
}

//Command Handler
void Lamp::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Lamp *lamp = (Lamp *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==lamp->genel.device_id || (dev_id==lamp->genel.register_id && dev_id>0)) {
            //Status var bunu kullan
            lamp->set_status(*st);
    } else {
                if (id==LAMP_ALL_ON) {
                        if (lamp->room_on) {
                            //printf("lamba on\n");
                            home_status_t ss = lamp->get_status();
                            lamp->tim_stop();
                            ss.stat = true;
                            lamp->set_status(ss);
                        }
                    }
                if (id==LAMP_ALL_OFF) {
                        //printf("lamba off\n");
                        home_status_t ss = lamp->get_status();
                        lamp->tim_stop();
                        ss.stat = false;
                        lamp->set_status(ss);
                    }

                if (id==ROOM_ON) lamp->room_on = true;
                if (id==ROOM_FON) {
                                     lamp->room_on = true;
                                     //odaya fiziki olarak girildi otel odası ise ve karşılama lambası ise 
                                     //çalıştır. 
                                     if (lamp->room_lamp)
                                       {
                                          if ((lamp->global & 0x02) == 0x02)
                                            {
                                                home_status_t ss = lamp->get_status();
                                                if (ss.active) {
                                                      ss.stat = true;
                                                      lamp->set_status(ss);
                                                      lamp->tim_stop();
                                                      lamp->tim_start();
                                                }
                                            }
                                       }
                                  }
                if (id==ROOM_OFF) {
                    lamp->room_on = false;
                    if (lamp->room_lamp) {
                       home_status_t ss = lamp->get_status();
                       if (ss.stat) {
                            lamp->tim_stop();
                            ss.stat = false;
                            lamp->set_status(ss);
                       } 
                    } 
                }  

                if (id==EMERGENCY_ON)
                {
                    lamp->store_set_status();
                    home_status_t ss = lamp->get_status();
                    if (!ss.stat)
                    {
                        ss.stat = true;
                        lamp->tim_stop();
                        lamp->set_status(ss); 
                        lamp->store_stat=true;
                    }
                }
                if (id==EMERGENCY_OFF)
                {
                    lamp->store_get_status();
                    home_status_t ss = lamp->get_status();
                    if (lamp->store_stat) lamp->set_status(ss); 
                    lamp->store_stat=false;
                }
                if (id==FIRE_ON) 
                {
                    if ((lamp->global&0x01)==0x01 && lamp->room_on)
                    {                       
                        if (++lamp->fire_count==FIRE_DELAY)
                            {
                                lamp->store_set_status();
                                home_status_t ss = lamp->get_status();
                                if (!ss.stat)
                                {
                                    ss.stat = true;
                                    lamp->tim_stop();
                                    lamp->set_status(ss); 
                                    lamp->store_stat=true;
                                    lamp->flash_on();

                                }
                            }
                    }
                }

                if (id==FIRE_OFF) 
                {
                    if ((lamp->global&0x01)==0x01)
                    {
                        lamp->flash_off();
                        lamp->store_get_status();
                        home_status_t ss = lamp->get_status();
                        if (lamp->store_stat) lamp->set_status(ss); 
                        lamp->store_stat=false;
                        lamp->fire_count = 0;
                    }
                }
    } 
}

//Sensor mesajları
void Lamp::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Lamp *lamp = (Lamp *) handler_args;
    Base_Port *target = lamp->port_head_handle;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0)
            {                
                if (!lamp->room_lamp) lamp->room_on=true;
                home_status_t ss = lamp->get_status();
                if (lamp->room_on && lamp->genel.active) 
                  {                     
                     if (strncmp(target->name,"AN",2)==0)
                       {
                          //Bu bir anahtardır
                          lamp->tim_stop();
                          ss.stat = st->stat;
                          lamp->set_status(ss);
                          lamp->send_virtual_anahtar(st->sender);
                       }  
                  }
                if (strncmp(target->name,"SN",2)==0)
                {
                    //Bu bir sensordur. stat 1 ise lamba açılır.Odanın aktif olduguna bakılmaz
                    if (st->stat && !ss.stat && lamp->genel.active) {
                        ss.stat = st->stat;
                        lamp->set_status(ss);
                        lamp->tim_stop();
                        lamp->tim_start(); 
                                              } 
                }  
            }
        target=target->next;
    }
}

void Lamp::ftim_callback(void* arg)
{ 
    Lamp *lamp = (Lamp *) arg;
    lamp->status.stat=false; 
    lamp->set_status_non_message(lamp->status);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    lamp->status.stat=true; 
    lamp->set_status_non_message(lamp->status);  
}

void Lamp::flash_on(void)
{
   // memcpy(&temp_status,&status,sizeof(home_status_t));
    //status.stat = true;
    //set_status_non_message(status);
    if (ftimer!=NULL)
     if (!esp_timer_is_active(ftimer))
       ESP_ERROR_CHECK(esp_timer_start_periodic(ftimer, 5 * 1000000)); 

}

void Lamp::flash_off(void)
{
   // memcpy(&status,&temp_status,sizeof(home_status_t));
    if (ftimer!=NULL)
      if (esp_timer_is_active(ftimer)) esp_timer_stop(ftimer);
    //set_status_non_message(status);
}

//Alarm mesajları
void Lamp::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Lamp *lamp = (Lamp *) handler_args;

    if (st->stat) {
        if ((lamp->global&0x01)==0x01) lamp->flash_on();
    } else {
        if ((lamp->global&0x01)==0x01) lamp->flash_off(); 
    }
    
}

void Lamp::init(void)
{
    if (!genel.virtual_device)
    { 
        esp_timer_create_args_t arg1 = {};
        arg1.callback = &xtim_callback;
        arg1.name = "Ltim1";
        arg1.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg1, &xtimer)); 

        esp_timer_create_args_t arg2 = {};
        arg2.callback = &ftim_callback;
        arg2.name = "Ltim1";
        arg2.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg2, &ftimer));
        
        if ((global & 0x04) == 0x04) room_lamp = true;
        //if ((global & 0x02) == 0x02) 
        {
            //Karşılama lambasıdır Timerı tanımla boşta beklesin 
            esp_timer_create_args_t arg = {};
            arg.callback = &lamp_tim_callback;
            arg.name = "Ltim0";
            arg.arg = (void *) this;
            ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
            if (duration==0) duration=30;
        }

        set_status(status);
         
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
    }
}

/*
   EMERGENCY_ON ile lamba yakılır OFF ile eski haline getirilir.

   Fonksiyonun json tanımı ve özellikleri
        {
            "name":"lamp",
            "id": 1,      
            "loc": 0,
            "subdev":0,          
            "timer": 10,
            "global": 2,
            "hardware": {
                "location" : "local",
                "port": [
                    {"pin": 1, "pcf":1, "name":"anahtar", "type":"PORT_INPORT"},
                    {"pin": 4, "pcf":1, "name":"sensor", "type":"PORT_SENSOR"},
                    {"pin": 1, "pcf":1, "name":"role", "type":"PORT_OUTPORT"}
                ]
            }
        }

    name : "lamp" olmak zorundadır.
    id   : tanım id sidir. Bu numara sistem tarafından verilir.
    loc  : fonksiyonun fiziki yerini tanımlar. (kullanılmıyor)
    subdev: (kullanılmıyor)
    timer : sensor tanımı varsa lambanın ne kadar yanık kalacagını belirtir. 
            0 verilirse default 30 alınır.
    global : bit bazında bir tanımlamadır. 
                ---- -XXX
             En sağdaki X lambanın yangın/Deprem ikaz lambası olduğunu gösterir.
             ikinci X ise karşılama lambası olduğunu gösterir.
             üçüncü X lambanın otel odası lambası oldugunu gösterir. 

        
             Buraya verilebilecek değerler:
                1  Yangın ikaz lambası
                2  Karşılama lambası
                3  Yangın ikaz ve karşılama lambası
                4  otel oda lambası vb

              Yangın/Deprem ikaz lambası : 
                                    Yangın veya deprem alarmı alındığında bu lamba sönükse yakılır 
                                    ve ana kontaktör kapanana kadar veya alarm iptal
                                    edilinceye kadar yanık kalır. Yanma flash
                                    seklinde olacaktır. (5sn aralıklarla 4sn yanık 1sn sonuk)
                                    
              Karşılama lambası : Sensör girişi aktif olduğunda lamba yakılır ve tanımlanan 
                                  süre sonunda söndürülür. Bekleme süresinde lamba manuel 
                                  olarak veya program aracılığı ile söndürülebilir. 
                                  - Oda normal oda ise karşılama lambasının çalışması için 
                                  sensor tanımı zorunludur. 
                                  - Otel oda lambası tanımlı ise ve sensor tanımı yoksa 
                                  oda fiziki olarak aktive olduğunda lamba çalıştırılır. 
                                    
              Otel oda lambası : Otel odası içinde, oda genel enerjisine baglı çalışan
                                 lamba anlamına gelir. Bu tanımlama yapılırsa ROOM_ON 
                                 mesajı alınmadan lamba anahtarlar ile aktive edilemez.
                                 Ancak yazılım ile aktif hale getirmek mümkün olur. 
                                 ROOM_OFF mesajı lambaların kapanmasına neden olur  
               
 
    hardware: Fiziki tanımların yapıldığı bölümdür
        location : fiziki transmisyonu tanımlar. (Sistem kendisi ayarlar.)
        port : fonsksiyon için gerekli in/out birimlerinin tanımlandığı bölümdür
               lamba için bir çıkış portu tanımlamak zorunludur. Diğer portlar isteğe bağlıdır.
               tanımlanabilecek portlar;
                   1. PORT_INPORT
                   2. PORT_SENSOR
                   3. PORT_OUTPORT
                   4. PORT_INPULS
                   5. PORT_VIRTUAL (
                                     Anahtarın veya sensorun remote olması durumunda kullanılır. 
                                     name tanımında belirtilen mesajın stat durumuna
                                     uygun olarak lambanın ON/OFF pozisyonunu ayarlar.
                                     {"name":"AN01","status":"{"stat":"ON"}} vb.
                                     Virtual ANAHTAR isimleri AN, Virtual SENSOR isimleri
                                     SN ile başlamak zorundadır.
                                     SENSOR ler PORT_SENSOR gibi çalışır. Yani on bilgisi
                                     alındığında lamba belirli süre aktive edilir. Off
                                     bilgisi gözardı edilir. 
                                     Anahtar ismi AN ile başlıyorsa ve pin 1 olarak
                                     tanımlanmış ise port fiziki olarak işlem gördükten 
                                     sonra objenin statusundaki stat bilgisi virtual
                                     anahtarın bulunduğu yere rs485 üzerinden gönderilir.
                                   )
                   
            pin : portun fiziki pin numarasıdır. Eğer port pcf üzerinde ise
                  PORT_INPORT için 1-20, PORT_OUTPORT için 1-12 olmalıdır. Eğer 
                  port CPU üzerinde ise CPU gpio numarası tanımlanmalıdır. 
            pcf : portun pcf veya cpu üzerinde olduğunu gösterir. pcf:0 tanımı
                  portun cpu üzerinde pcf:1 tanımı pcf üzerinde olduğunu söyler
            name : Portun ismidir. Eger port virtual olarak tanımlanmış ise
                   hangi sensor veya anahtarın mesajlarını alacagı isim olarak
                   tanımlanır  
            type : portun tipini gösterir. PORT_INPORT,PORT_OUTPORT vb.
            reverse : INPORT tipi portlar eğer cpu üzerinde ise aksiyonun 
                      hangi kenarda gerçekleşecegini tanımlar. Default aksiyon 
                      0 dır. Eğer porta 1 geldiğinde aksiyon almasını istiyorsanız
                      reverse:1 tanımını yapınız. Virtual portlarda reverse tanımı yapılamaz                         

    Lamba için birden çok giriş veya birden çok çıkış portu tanımlanabilir. 
      -   Otel oda lambası ise, Oda açılmadan oda içindeki anahtarlar ile lamba aktif edilemez.
      -   Lamba iptal edilebilir. Yazılım ile iptal olan lamba fiziki olarak çalışmaz.
      -   Birden çok çıkış portu tanımlayarak gurup lamba  oluşturabilirsiniz. 
          (tek anahtara baglı çoklu lamba. bahçe aydınlarması vb)
      -   INPULS olmak kaydı ile birden çok anahtar bağlayarak vaviyen bağlantı sağlanabilir. 
      -   SENSOR portları kullanılarak alarm sistemlerinde otomatik aydınlatma 
          sağlanabilir. (kapı sensörü ile karşılama lambası, PIR ile güvenlik aydınlatması vb)  
      -   VIRTUAL Anahtarlar ile remote anahtarlar tanımlanabilir.  
      -   VIRTUAL Anahtarlar ile LDR sensor baglanabilir.  
      
      -   Hedef :
              - Zamana bağlı On/Off
              

    Lamba set_status fonksiyonu ile;
        1. stat=true, stat=false ile açılıp kapatılabilir. 
        2. active=true/false ile iptal edilebilir. 
        3. counter>0 özelliği ile belirlenen süre kadar açık bırakılır.
         
*/
