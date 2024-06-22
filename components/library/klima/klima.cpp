

#include "esp_log.h"
#include "klima.h"
#include <math.h>

static const char *KLIMA_TAG = "KLIMA";

typedef union {
    struct {
        uint16_t a0;
        uint16_t a1;
        uint16_t a2;
        uint16_t a3;
          } a;
     uint64_t b;
} long_t;

void Klima::rate(home_status_t st)
{
    local_set_status(st,true);
        //memcpy(&status,&st,sizeof(home_status_t));
        //  printf("Rate 0 Color %llX\n",status.color); 
    rate(); 
}
void Klima::rate(void)
{
    rapor_create();
    long_t zz;
    zz.b = status.color;
    //zz.a.a0 da degerler, zz.a.a1 de mask var
    //Sensore 0 geliyorsa sensor portuna GND gelmiyordur. Aynı olay 0.bit için de gecerli. 
    uint16_t val = (zz.a.a0&zz.a.a1)>>1;
    uint16_t mask = zz.a.a1>>1;
    bool sens = true;
    //maskı olan bir sayının degeri 0 ise sens=0 olacaktır. 
    //bu durumda klimanın kapatılması gerekir.

    for (int i=0;i<16;i++)
      {
        uint16_t vv = pow(2,i);
        if ((mask&vv)==vv) if ((val&vv)!=vv) sens=false;
      }

   // printf("Sens %d room %d\n",sens,room_on);


    if (sens) {
        //Klima açılıyor
       if (esp_timer_is_active(qtimer)) {
         ESP_LOGI(KLIMA_TAG,"Klima %d Kapatma iptal edidi",genel.device_id);
         tim_stop();
       } 
        
       if (!status.stat && (room_on || remote_start))
        {
            status.stat = true;
            set_status(status);
            ESP_LOGI(KLIMA_TAG,"Klima %d Aciliyor",genel.device_id);
        } 
        
    } else {
       //klima kapatılıyor
       if (status.stat==true)
       {
            tim_start();
            ESP_LOGI(KLIMA_TAG,"Klima %d Kapatiliyor",genel.device_id);
       }
    }
}

void Klima::tim_stop(void){
    if (qtimer!=NULL)
      if (esp_timer_is_active(qtimer)) esp_timer_stop(qtimer);
}
void Klima::tim_start(void){
    if (qtimer!=NULL)
     if (!esp_timer_is_active(qtimer))
       ESP_ERROR_CHECK(esp_timer_start_once(qtimer, duration * 1000000));
}

void Klima::remote_tim_stop(void){
    if (remote_timer!=NULL)
      if (esp_timer_is_active(remote_timer)) {
        esp_timer_stop(remote_timer);
        remote_start = false;
      }
}
void Klima::remote_tim_start(void){
    if (remote_timer!=NULL)
     if (!esp_timer_is_active(remote_timer))
       ESP_ERROR_CHECK(esp_timer_start_once(remote_timer, 60 * 5 * 1000000)); //5 dakika
}

void Klima::func_callback(void *arg, port_action_t action)
{
   if (action.action_type==ACTION_INPUT)
     {
        Klima *klm = (Klima *) arg;
        ESP_LOGI(KLIMA_TAG,"Kapı/Pencere durum değişikliği %d",klm->genel.active);
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_IN_EVENTS, MOVEMEND, (Base_Function *)arg, sizeof(Base_Function), portMAX_DELAY));
        if (klm->genel.active) klm->rate(); 
     }
}


void Klima::tim_callback(void* arg)
{ 
    Klima *klm = (Klima *) arg;
    home_status_t ss = klm->get_status();
    ss.stat = false;
    klm->set_status(ss);  
    ESP_LOGI(KLIMA_TAG,"Klima %d KAPATILDI",klm->genel.device_id); 
}


//bu fonksiyon klimayı yazılım ile tetiklemek için kullanılır.
void Klima::set_status(home_status_t stat)
{
    if (!genel.virtual_device)
    {
        local_set_status(stat);
        if (stat.active!=genel.active) genel.active = stat.active;
        if (!genel.active) status.stat = false;
       
        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT)
                {
                    target->set_status(status.stat);
                }
            target = target->next;
        }     
        write_status();
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(Klima), portMAX_DELAY));
    } else {
        ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(Klima), portMAX_DELAY));
    }
}


void Klima::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj, "color", stt.color);
    cJSON_AddNumberToObject(obj, "oid", genel.oid);
}

void Klima::get_status_json(cJSON* obj)
{

    rapor_create();
    return ConvertStatus(status , obj);
}

uint64_t Klima::bit_set(uint64_t number, uint8_t n) {
    return number | ((uint64_t)1 << n);
}
uint64_t Klima::bit_clear(uint64_t number, uint8_t n) {
    return number & ~((uint64_t)1 << n);
}
uint64_t Klima::bit_status(uint64_t number, uint8_t n, uint8_t val)
{
    uint64_t aa = (val) ? bit_set(status.color,n) : bit_clear(status.color,n);
    return aa;   
}

void Klima::printBits(size_t const size, void const * const ptr)
{
    unsigned char *b = (unsigned char*) ptr;
    unsigned char byte;
    int i, j;
    
    for (i = size-1; i >= 0; i--) {
        for (j = 7; j >= 0; j--) {
            byte = (b[i] >> j) & 1;
            printf("%u", byte);
        }
    }
    puts("");
}

void Klima::rapor_create(void)
{
        //printf("Rapor Start %llX\n",status.color); 

        Base_Port *target = port_head_handle;
        while (target) {
            if (target->type == PORT_OUTPORT)
              {
                uint8_t pd = !target->get_hardware_status();
                status.color = bit_status(status.color,0,pd);
                status.color = bit_status(status.color,16,1);
                break;
              }
            target = target->next;
                        }
        target = port_head_handle;
        uint8_t ii=1;
        while (target) {
            if (target->type == PORT_INPORT)
              {
                uint8_t pd = !target->get_inport_status();
                
                //printf("ps %d %s %d\n",ii,target->name,pd);
                
                status.color = bit_status(status.color,ii,pd);
                status.color = bit_status(status.color,ii+16,1);
                ii++; 
              }
            if (target->type == PORT_VIRTUAL)
              {
                //printf("ps %d %s %d\n",ii,target->name,0);
                //printf("ii=%d\n",ii);
                status.color = bit_status(status.color,ii+16,1);  
                ii++;               
              }  
             
            target = target->next;
            
                        }
        //printf("Rapor Stop %llX\n",status.color); 
        //printf("color=%llX\n",status.color);   
        //printBits(sizeof(uint64_t),&status.color);            
}


//Sensor mesajları
void Klima::virtual_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Klima *klm = (Klima *) handler_args;
    Base_Port *target = klm->port_head_handle;
    uint8_t i=1;
    while (target) {
        if (strcmp(target->name,(char*)st->name)==0 && target->type == PORT_VIRTUAL)
            {                
                home_status_t ss = klm->get_status();
                //printf("%s %d\n",st->name,st->stat);
                if (klm->genel.active) 
                  {                     
                     if (strncmp(target->name,"AN",2)==0)
                       {  
                          uint8_t pd = (st->stat)?1:0;
                          //printf("i=%d pd=%d %llX\n",i,pd,ss.color); 
                          ss.color = klm->bit_status(ss.color,i,pd);
                          //printf("i=%d pd=%d %llX\n",i,pd,ss.color);
                          klm->rate(ss);
                       } 
                     if (strncmp(target->name,"SN",2)==0)
                       {
                          klm->remote_temp = st->temp;
                       }   
                  }
            }
        i++;    
        target=target->next;
    }
}

//Alarm mesajları
void Klima::alarm_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_virtual_t *st = (home_virtual_t *) event_data;
    Klima *klm = (Klima *) handler_args;
    if (st->stat) {
        klm->store_set_status();
        klm->remote_tim_stop();
        home_status_t ss = klm->get_status();
        ss.stat=false;
        klm->set_status(ss);
    } else {
        klm->store_get_status();
        home_status_t ss = klm->get_status();
        klm->set_status(ss); 
    } 
}

void Klima::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    Klima *klm = (Klima *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==klm->genel.device_id || id==ROOM_ON || id==ROOM_OFF || id==ROOM_FON || id==CHECK_IN || id==CHECK_OUT) {
        if (dev_id>0) {
            //Status var bunu kullan
            klm->set_status(*st);     
        } else {  
                if (id==ROOM_ON) klm->room_on = true;
                if (id==ROOM_FON) {
                                     klm->room_on = true;
                                     //odaya fiziki olarak girildi kontrol et ve çalıştır.
                                     ESP_LOGI(KLIMA_TAG,"Klima Kontrol ediliyor");
                                     klm->remote_tim_stop();
                                     klm->rate();
                                  }
                if (id==ROOM_OFF) {
                    klm->room_on = false;
                    klm->remote_tim_stop();
                    home_status_t ss = klm->get_status();
                    ss.stat = false;                    
                    klm->set_status(ss);                     
                } 
                if (id==CHECK_IN) {
                    //Checkin yapıldığında oda ısısı belirlenen ısının üstünde ise
                    //Klimayı 5dk süre ile çalıştır.
                    if (klm->remote_temp==0) klm->remote_temp=35;
                    ESP_LOGI(KLIMA_TAG,"Check in Algılandı. Set=%d Derece=%d",klm->remote_set_temp,klm->remote_temp);
                    if (klm->remote_temp>klm->remote_set_temp)
                    {
                        home_status_t ss = klm->get_status();
                        ss.stat = true;
                        klm->set_status(ss); 
                        klm->remote_start = true;
                        klm->remote_tim_stop();
                        klm->remote_tim_start();   
                    }                      
                }
                if (id==CHECK_OUT) {
                    if (!klm->room_on)
                      {
                            ESP_LOGI(KLIMA_TAG,"Check Out Algılandı");
                            home_status_t ss = klm->get_status();                            
                            ss.stat = false;
                            klm->set_status(ss); 
                            klm->remote_tim_stop();
                            klm->remote_start = false;                            
                      }
                } 
        }
    } 
}

void Klima::init(void)
{
    if (!genel.virtual_device)
    {
        esp_timer_create_args_t arg = {};
        arg.callback = &tim_callback;
        arg.name = "Ltim0";
        arg.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg, &qtimer)); 
        if (duration==0) duration=30;

        esp_timer_create_args_t arg1 = {};
        arg1.callback = &tim_callback;
        arg1.name = "Ltim1";
        arg1.arg = (void *) this;
        ESP_ERROR_CHECK(esp_timer_create(&arg1, &remote_timer)); 
        if (duration==0) duration=30;

        remote_tim_stop();
      
        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        ESP_ERROR_CHECK(esp_event_handler_instance_register(ALARM_EVENTS, ESP_EVENT_ANY_ID, alarm_handler, (void *)this, NULL));
        ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        rate();      
    }
}


/*
     BETA TEST OK
     
     Klima objesi bir sensore baglı olarak, klimanın çalışmasına izin veren bir yapıdır. 
     Otel odalarının klimasını oda aktivitesi, kapı ve pencerelere baglı olarak durdurur 
     veya çalışmasına izin verir.

     OLASI TANIM

        {
            "name":"klima",
            "uname":"Klima 1",
            "id": 1,
            "loc": 0,     
            "timer": 20,
            "global":0,
            "hardware": {
                "port": [
                    {"pin": 4, "pcf":1, "name":"Kapı 1", "type":"PORT_INPORT"},
                    {"pin": 5, "pcf":1, "name":"Kapı 2", "type":"PORT_INPORT"},
                    {"pin": 0, "pcf":0, "name":"ANxx", "type":"PORT_VIRTUAL"}, //Kapı pencere anahtarları
                    {"pin": 3, "pcf":1, "name":"KLM role", "type":"PORT_OUTPORT"},
                    {"pin": 0, "pcf":0, "name":"SNxx", "type":"PORT_SENSOR"} //Isı sensorleri
                ]
            }
        } 

    Yazılım yolu ile iptal edilebilir veya sensorlere baglı olmaksızın sürekli çalışmasına izin 
    verilebilir. 

    Virtual sensor veya anahtar tanımlanabilir. (Anahtar ismi AN, sensor ismi SN ile baslamalıdır)

    Genel çalışma mantıgı:
           ODA_ON olması durumunda diğer unsurlar uygun ise klima ON yapılır. Aksi durumda 
        klima kapatılacaktır. 
           Klima çalışırken sensorlerden herhangi bir aktif olur ise timer ile belirlenen 
        süre kadar beklendikten sonra klima kapatılır. Bekleme anında sensor normal
        konumuna gelirse kapatma işlemi iptal edilir. 
           Sensorlerin virtual olması durumunda sensorun gönderdiği degerler status.color
        degişkeninde saklanır ve son gönderdiği deger aktif deger olarak varsayılır.
                     
    Röle ve sensorlerin durumu rapor edilir. Rapor status.color üzerinden bit 
    bazında yapılacaktır. Color 64 bit olup ilk 32 biti kullanılacaktır. 
    Color yapılanması ilk 16 bit ve 2.16 bit şeklinde olacaktır. 
    ilk 16 bit 0.bit çıkış portu olmak üzere port durumlarını 2. 16bit ise ilk 16 bitin mask'ı
    olacaktır. Örnegin
             HW  0000000000000111  LW 0010100000000101 yapılanmasında
             HW&LW 5 çıkacaktır. Bu 5 sayısının anlamı 
             0.bit çıkış portu 1
             1.bit 1.sensor durumu 0
             2.bit 2.sensor durumu 1
    Bu rapor ile klima objesinin durumu programlar tarafından izleyene gösterilebilir. 

    Klima objesine ısı yayınlayan sensör tanımlanabilir. Algılanan ısı checkin sırasında 
    kullanılır. 

*/