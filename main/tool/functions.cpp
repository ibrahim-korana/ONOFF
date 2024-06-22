
const char *FUNTAG = "FUNCTIONS";

#define ARDUINOJSON_ENABLE_COMMENTS 1

#include "classes.h"
#include "ArduinoJson.h"
#include "room.h"
#include "lamp.h"
#include "klima.h"
#include "air.h"
#include "kontaktor.h"
#include "gas.h"
#include "water.h"
#include "asansor.h"
#include "bell.h"
#include "curtain.h"
#include "onoff.h"
#include "hdoor.h"
#include "security.h"
#include "emergency.h"
#include "dnd.h"
#include "clnok.h"
#include "checkin.h"
#include "roomalarm.h"
#include "fire.h"
#include "Message.h"
#include "lampon.h"
#include "dayclean.h"

Base_Function *function_head_handle = NULL;
uint8_t Function_Counter = 0;
prg_location_t *locations = NULL;

#define PORT_DEBUG false

bool out_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin)
{
  bool ret=false;
  switch (pin)
   {
      case 1: {*pcfno=0;*pcfpin=ROLE1;ret=true;break;}
      case 2: {*pcfno=0;*pcfpin=ROLE2;ret=true;break;}
   }
  return ret;
}

bool in_pin_convert(uint8_t pin, uint8_t *pcfno, uint8_t *pcfpin)
{
  bool ret=false;
  switch (pin)
   {
      case 1: {*pcfno=0;*pcfpin=TUS1;ret=true;break;}
      case 2: {*pcfno=0;*pcfpin=TUS2;ret=true;break;}
   }
  return ret;
}


Base_Function *get_function_head_handle(void)
{
  return function_head_handle;
}
prg_location_t *get_location_head_handle(void)
{
  return locations;
} 


void *add_function(uint8_t a_icon, const char *a_name, const char *au_name, uint8_t loc, Storage dsk)
{
    void *bb0 = NULL;
    uint8_t uid = ++Function_Counter;
    //---------------------  
      if (strcmp(a_name,"room")==0) bb0 = (Room *)new Room(uid, dsk);             //Test Ok
      if (strcmp(a_name,"lamp")==0) bb0 = (Lamp *)new Lamp(uid, dsk);             //Test OK

      if (strcmp(a_name,"klima")==0) bb0 = (Klima *)new Klima(uid, dsk);          //Test OK
      
      if (strcmp(a_name,"air")==0) bb0 = (Air *)new Air(uid, dsk);
      if (strcmp(a_name,"cont")==0) bb0 = (Contactor *) new Contactor(uid, dsk); //Test OK
      if (strcmp(a_name,"gas")==0) bb0 = (Gas *) new Gas(uid,dsk);               // Test OK
      if (strcmp(a_name,"water")==0) bb0 = (Water *) new Water(uid, dsk);        // Test OK
      if (strcmp(a_name,"elev")==0) bb0 = (Asansor *) new Asansor(uid, dsk);     // Test OK
      if (strcmp(a_name,"bell")==0) bb0 = (Bell *) new Bell(uid, dsk);           // Test OK  
      if (strcmp(a_name,"cur")==0) bb0 = (Curtain *) new Curtain(uid,dsk); 
      if (strcmp(a_name,"onoff")==0) bb0 = (Onoff *) new Onoff(uid, dsk);        //Test OK
      if (strcmp(a_name,"hdoor")==0) bb0 = (HDoor *) new HDoor(uid, dsk);        //Test OK
      if (strcmp(a_name,"sec")==0) bb0 = (Security *) new Security(uid, dsk); 
      if (strcmp(a_name,"emergency")==0) bb0 = (Emergency *) new Emergency(uid, dsk); //Test ok
      if (strcmp(a_name,"dnd")==0) bb0 = (Dnd *) new Dnd(uid, dsk);
      if (strcmp(a_name,"clnok")==0) bb0 = (ClnOK *) new ClnOK(uid, dsk);
      if (strcmp(a_name,"checkin")==0) bb0 = (Checkin *) new Checkin(uid, dsk);
      if (strcmp(a_name,"ralarm")==0) bb0 = (RoomAlarm *) new RoomAlarm(uid, dsk);
      if (strcmp(a_name,"fire")==0) bb0 = (Fire *) new Fire(uid, dsk);
      if (strcmp(a_name,"message")==0) bb0 = (Message *) new Message(uid, dsk);
      if (strcmp(a_name,"lampon")==0) bb0 = (Lampon *) new Lampon(uid, dsk);
      if (strcmp(a_name,"dayclean")==0) bb0 = (DayClean *) new DayClean(uid, dsk);
            
    /*     
      if (strcmp(a_name,"priz")==0) bb0 = (Priz *) new Priz(a_id, fun_cb, dsk);                           
      if (strcmp(a_name,"sec")==0) bb0 = (Security *) new Security(a_id,fun_cb, dsk);            
      if (strcmp(a_name,"tmrelay")==0) bb0 = (TmRelay *) new TmRelay(a_id,fun_cb, dsk);
      if (strcmp(a_name,"pis")==0) bb0 = (Piston *) new Piston(a_id,fun_cb, dsk);
      */
    //---------------------
    if (bb0!=NULL)    
      {
        ((Base_Function *)bb0)->genel.device_id = uid;
        ((Base_Function *)bb0)->genel.icon = a_icon;
        strcpy(((Base_Function *)bb0)->genel.uname, au_name);
        //------ fonksiyonu listeye ekle -------------
        ((Base_Function *)bb0)->next = function_head_handle;
        ((Base_Function *)bb0)->location = loc;

        function_head_handle = (Base_Function *)bb0;
      }
    return bb0;  
}

void add_port(JsonArray port, Base_Function *cls, Dev_8574 pcf[], bool debug=false)
{

   for (JsonObject prt : port) {
      int _p_port = prt["pin"]; // 1, 1
      const char* _p_name = prt["name"]; // "anahtar", "role"
      const char* _p_type = prt["type"]; // "PORT_INPORT", ...
      int _p_rev = prt["reverse"];
      int _p_pcf = prt["pcf"];

      port_type_t tt = port_type_convert((char *)_p_type); 
      Base_Port *aa = new Base_Port();
      assert(aa!=NULL);
      strcpy(aa->name,_p_name);
      aa->Klemens = _p_port;
      if (tt==PORT_OUTPORT) {
                    /*pcf üzerinden out lar*/
                    if (_p_pcf>0) {
                        uint8_t pcfno=0,pcfpin=0;
                        if (out_pin_convert(_p_port,&pcfno,&pcfpin))
                          {  
                              out_config_t *cfg = (out_config_t *)malloc(sizeof(out_config_t));

                             // printf("OUT PORT CFG %d %d %s\n",pcfno, pcfpin, _p_name);

                              cfg->type = OUT_TYPE_EXPANDER;
                              uint8_t al=0;
                              if (_p_rev==1) {al=1;}
                              cfg->pcf8574_out_config = {};
                              cfg->pcf8574_out_config.pin_num = pcfpin;
                              cfg->pcf8574_out_config.device = &pcf[pcfno];
                              cfg->pcf8574_out_config.reverse = al;
                                  
                              out_handle_t cikis = iot_out_create(cfg); 
                              aa->set_outport(cikis);  
                          }
                  } else {
                    /*gpio üzerinden yapılan outlar*/
                    out_config_t *cfg = (out_config_t *)malloc(sizeof(out_config_t));
                    cfg->type = OUT_TYPE_GPIO;
                    cfg->gpio_out_config = {
                        .gpio_num = _p_port,
                        }; 
                    out_handle_t cikis = iot_out_create(cfg); 
                    aa->set_outport(cikis);  
                  }
                  aa->set_port_type(tt,(void*)cls);  
                  cls->add_port(aa,PORT_DEBUG);
              }

      if (tt!=PORT_OUTPORT) {
        if (_p_pcf>0) {
          //port pcf üzerinde
          uint8_t pcfno=0,pcfpin=0;
          if (in_pin_convert(_p_port,&pcfno,&pcfpin))
            {
              //pin numarası doğru
              button_config_t *cfg = (button_config_t *)malloc(sizeof(button_config_t));
              cfg->type = BUTTON_TYPE_EXPANDER;
              cfg->pcf8574_button_config = {};
              cfg->pcf8574_button_config.pin_num = pcfpin;
              cfg->pcf8574_button_config.device = &pcf[pcfno];

             //  printf("Eklenen Port %x\n",cfg->pcf8574_button_config.device);

              cfg->pcf8574_button_config.active_level = 0;
              button_handle_t giris = iot_button_create(cfg);
              aa->set_inport(giris); 
              aa->set_port_type(tt,(void*)cls);  
              cls->add_port(aa,PORT_DEBUG); 
            }
                      } else {
                        if (tt!=PORT_VIRTUAL)
                        {
                        //port gpio üzerinde
                        uint8_t lev = 0;
                        if (_p_rev==1) lev=1;
                        button_config_t *cfg = (button_config_t *)malloc(sizeof(button_config_t));
                        cfg->type = BUTTON_TYPE_GPIO;
                        cfg->gpio_button_config = {
                                  .gpio_num = _p_port,
                                  .active_level = lev,
                              };
                        button_handle_t giris = iot_button_create(cfg);
                        aa->set_inport(giris); 
                        aa->set_port_type(tt,(void*)cls);  
                        cls->add_port(aa,PORT_DEBUG);   
                        } else {
                          //port Virtualdır
                          aa->virtual_port = true;
                          aa->type = tt; 
                          aa->ack = (_p_port==0) ? false : true;
                          cls->add_port(aa,PORT_DEBUG); 
                        }
                      }

      }        
  } //for
}

void *Read_functions(
                       Storage dsk,
                       Dev_8574 pcf[]
                     )
{
    //printf("BEFORE Free heap: %u\n", esp_get_free_heap_size());

    const char *name1="/config/config.json";
    if (dsk.file_search(name1))
      {
        int fsize = dsk.file_size(name1); 
       ESP_LOGI("READ_FUNCTION"," File SIZE    : %u" , fsize);
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE(FUNTAG, "memory not allogate"); return NULL;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE(FUNTAG, "%s not open",name1); return NULL;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        
        DynamicJsonDocument doc(fsize+5);
        DeserializationError error = deserializeJson(doc, buf);
       
        if (error) {
          ESP_LOGE(FUNTAG,"deserializeJson() failed: %s",error.c_str());
          return NULL;
        }

        for (JsonObject function : doc["functions"].as<JsonArray>()) {
          const char* a_name = function["name"];
          const char* au_name = function["uname"];
          int a_icon = function["icon"]; 
          int a_loc = function["loc"]; 
          int a_tim = function["timer"]; 
          int a_glo = function["global"];
          //const char* function_hardware_location = function["hardware"]["location"]; 

          void *bb0 = add_function(a_icon, a_name, au_name, a_loc, dsk);
          if (bb0!=NULL)    
              {
                ((Base_Function *)bb0)->duration = a_tim;
                ((Base_Function *)bb0)->global = a_glo;
                ((Base_Function *)bb0)->genel.location = TR_LOCAL;
                add_port(function["hardware"]["port"].as<JsonArray>(),((Base_Function *)bb0),pcf);
               
                //if (strcmp(a_name,"air")==0) ((Base_Function *)bb0)->list_port();
              }  
        }

        

        doc.clear();                       
        free(buf);
        uint8_t kk=0; 
        Base_Function *target = function_head_handle;
        while(target)
          {
            ((Base_Function *)target)->init();
           // ((Base_Function *)target)->set_active(true);
            kk++;
            target=((Base_Function *)target)->next;
          }

        ESP_LOGI(FUNTAG,"config.json dosyasından %d fonksiyon eklendi ve başlatıldı.",kk);           
      }
     
    //printf("AFTER Free heap: %u\n", esp_get_free_heap_size());
    return NULL;   
}

uint8_t  function_list(bool prn)
{
    Base_Function *target = function_head_handle;
    uint8_t nreg = 0;
    if (prn) ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    if (prn) ESP_LOGI(FUNTAG,"     DEV REG CIH NAME           AUNAME         STR LOC");
    if (prn) ESP_LOGI(FUNTAG,"     ID  ID  ID  ");
    if (prn) ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"); 
    while(target)
      {
        Base_Function *cc = (Base_Function *)target;
        if (prn)
              ESP_LOGI(FUNTAG,"     %3d %3d %3d %-15s %-15s %3d %3d", 
                    cc->genel.device_id,
                    cc->genel.register_id,
                    cc->genel.register_device_id,
                    cc->genel.name, 
                    cc->genel.uname, 
                    cc->get_status().active,
                    cc->location
                    );
          if (cc->genel.register_id==0 && cc->genel.register_device_id==0) nreg++;           
          target=cc->next;
      }
    if (prn) ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");    
    return nreg;  
}

uint8_t function_count(void)
{
  uint8_t cnt=0;
  Base_Function *target = function_head_handle;
  while(target)
      {
          cnt++;
          target=target->next;
      }
  return cnt;
}

//--------------------------------------------------
void add_locations(prg_location_t *lc )
{
  lc->next = (prg_location_t *)locations;
  locations = lc;
}

void list_locations(void)
{
  prg_location_t *target = locations;
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
    ESP_LOGI(FUNTAG,"     LOCATIONS");
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~"); 
    while(target)
      {
        ESP_LOGI(FUNTAG,"     %3d %-20s", 
              target->page,
              target->name
              );
          target=(prg_location_t *)target->next;
      }
    ESP_LOGI(FUNTAG,"     ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");  
}

void *read_locations(Storage dsk)
{
    const char *name1="/config/location.json";
    if (dsk.file_search(name1))
      {
        int fsize = dsk.file_size(name1); 
        char *buf = (char *) malloc(fsize+5);
        if (buf==NULL) {ESP_LOGE(FUNTAG, "memory not allogate"); return NULL;}
        FILE *fd = fopen(name1, "r");
        if (fd == NULL) {ESP_LOGE(FUNTAG, "%s not open",name1); return NULL;}
        fread(buf, fsize, 1, fd);
        fclose(fd);
        DynamicJsonDocument doc(3000);
        DeserializationError error = deserializeJson(doc, buf);

        if (error) {
          ESP_LOGE(FUNTAG,"deserializeJson() failed: %s",error.c_str());
          return NULL;
        }

        for (JsonObject function : doc["locations"].as<JsonArray>()) {
          const char* a_name = function["name"];
          int a_page = function["page"]; 
         
          prg_location_t *bb0 = new prg_location();
          strcpy(bb0->name,a_name);
          bb0->page = a_page;
          add_locations(bb0);
        }
      
        doc.clear();                       
        free(buf);
      }
   return NULL;   
}

Base_Function *function_find(uint8_t id)
{
    Base_Function *target = function_head_handle;
    while(target)
      {
        Base_Function *cc = (Base_Function *)target;
        if (cc->genel.device_id == id) return cc;
        target=cc->next;
      }
    return NULL;
}

Base_Function *find_register(uint8_t sender, uint8_t rid)
{
    Base_Function *target = function_head_handle;
    while(target)
      {
        Base_Function *cc = (Base_Function *)target;
        if (cc->genel.register_device_id == sender && cc->genel.register_id==rid) return cc;
        target=cc->next;
      }
    return NULL;
}
