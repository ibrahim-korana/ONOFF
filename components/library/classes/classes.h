
#ifndef _CLASSES_CLASSES_H_
#define _CLASSES_CLASSES_H_

#include "iot_out.h"
#include "iot_button.h"
#include "core.h"
#include "storage.h"
#include "cJSON.h"

class Base_Port
{
    public:
       Base_Port() {
    	    name=(char*)calloc(1,20);
    	    strcpy(name,"NO_NAME");
    	    active = true;
    	    user_active = true;
    	    alarm = false;

    	};
       ~Base_Port() {
           free(name);
       };
       void set_outport(out_handle_t ou) {outport = ou;}
       void set_inport(button_handle_t inp) {inport = inp;}

       bool set_port_type(port_type_t tp, void *usr_data);
       void set_active(bool dr);
       void set_active_admin(bool dr);
       bool get_active(void) {return active;}
       void set_user_active(bool dr);
       bool get_user_active(void) {return user_active;}
       bool set_status(bool st);
       uint16_t set_color(uint16_t color);
       bool set_toggle(void);
       bool get_hardware_status();
       bool get_inport_status();
       bool get_alarm(void) {return alarm;}
       void set_alarm(bool al) {alarm=al;}
       void set_callback(void* bas, port_callback_t cb) {
          base_function=bas;
          port_action_callback = cb;
        }

       char *name;
       uint8_t id=0;
       port_type_t type;
       bool status;
       bool virtual_port=false;
       void *base_function;
       bool ack=false;

       out_handle_t outport = NULL;
       button_handle_t inport = NULL;

       uint8_t Klemens;
       Base_Port *next=NULL;

       protected:
        /*
           in port'da istenen aksiyon oluştuğunda lokal olarak bu fonksiyon çağırılır
           bu callback de gerekli düzenlemeleri yaparak port_action_callback üzerinden
           aksiyonu dışarıya aktarır
        */
        static void inportscall(void *arg, void *usr);
        /*
            out portta bir aksiyon  oluştuğunda lokal olarak bu fonksiyon çağırılır
            bu callback de gerekli düzenlemeleri yaparak port_action_callback üzerinden
            aksiyonu dışarıya aktarır
        */
        static void outportscall(void *arg, void *usr);
         /*active portun işlevini yerine getirmesini veya tamamen kapanmasını sağlar*/
         bool active = true;
         /*
         User  Active portun kullanıcı tarafından iptal edilmesini sağlar.
         User Active true olmayan bir port start/stop olamaz.
         */
         bool user_active = true;
         /*
         Portta aksiyon oluştuğunda bu fonksiyon çağrılır.
         böylece oluşan degişim fonksiyona aktarılır. Bu callback portun baglı oldugu
         fonksiyon tarafından set_callback çağırılarak oluşturulur.
         */
         port_callback_t port_action_callback=NULL;
         bool alarm = false;
};


class Base_Function
{
    public:
      function_prop_t genel;
      uint8_t duration;
      uint8_t global;
      uint8_t location;
      Base_Function() {
              genel.location = TR_NONE;
              duration = 0;
              global=0;
              genel.register_id = 0;
              genel.register_device_id = 0;
              genel.virtual_device=false;
              genel.registered=false;
              genel.active = true;
              genel.icon = 0;
              strcpy(genel.name,"");
              strcpy(genel.uname,"");
            };
      ~Base_Function() {
              //free(name);
            };
      Base_Function *next = NULL;

      Base_Port *port_head_handle = NULL;
      uint8_t port_count=0;
      bool room_on = false;
      void add_port(Base_Port *dev, bool debug=false);
      void list_port(void);


            bool get_active(void) {return genel.active;}

            	/*
                fonksiyonda oluşan tüm değişimlerde ilgili yerlere haber vermek üzere
                (program veya diğer fonksiyonlar) bu callback çağırılır. bu callback
                ana program tarafından konulur.
            	 */
            function_callback_t function_callback = NULL;
            	/*
                Base_fonction nesensini devralan sınıflar local_port_callback
                tanımlayarak input ve output aksiyonlarını alabilirler.
            	 */
            port_callback_t local_port_callback = NULL;
            	/*
                Eger fonksiyon virtual olarak tanımlanmış ise gelen değişim
                emirlerini alt veya üst device a aktarabilmek için bu callback
                kullanılır.
            	 */
            function_callback_t command_callback = NULL;

            bool local_set_status(home_status_t stat, bool write=false)
			  {   bool chg=false;
                  if (stat.stat!=status.stat) chg=true;
                  if (stat.status!=status.status) chg=true;
                  if (stat.active!=status.active) chg=true;
                  if (stat.temp!=status.temp) chg=true;
                  if (stat.set_temp!=status.set_temp) chg=true;
                  if (stat.color!=status.color) chg=true;
                  if (stat.counter!=status.counter) chg=true;
				  memcpy(&status,&stat,sizeof(home_status_t));
                  if (stat.active!=genel.active) genel.active = stat.active;
				  if (write) write_status();
                  return chg;
			  }

            home_status_t get_status(void) {return status;};
            void store_set_status(void) {
                memcpy(&main_temp_status,&status,sizeof(home_status_t));
                
               // printf("SAKLANAN STAT %d %d\n",genel.device_id,main_temp_status.stat);

            }
            void store_get_status(void)
            {
                memcpy(&status,&main_temp_status,sizeof(home_status_t));
               // printf("GETIRILEN STAT %d %d\n",genel.device_id,status.stat);
            }
            virtual void init(void) = 0;
            virtual void get_status_json(cJSON* obj) = 0;
            virtual bool get_port_json(cJSON* obj) {return false;};
            virtual void get_port_status_json(cJSON* obj) {};

        void function_set_register(uint8_t id, uint8_t dev) {
        //if(esp_timer_is_active(reg_tim)) esp_timer_stop(reg_tim);
        genel.register_id = id;
        genel.register_device_id = dev;
        genel.registered = true;
         }
            
            //virtual void senaryo(char *par) {};
    protected:
            //Türetilenden erişilir
            home_status_t status;
            home_status_t main_temp_status;
            Storage disk;
            void write_status(void)
			  {
				  disk.write_status(&status, genel.device_id);
			  };

    private:
};



#endif
