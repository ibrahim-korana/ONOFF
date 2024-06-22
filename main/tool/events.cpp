#include "classes.h"


void function_out_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //ESP_LOGW(TAG, "LED %ld %ld", id , base, id);
    Base_Function* func = (Base_Function*) event_data;
    uint32_t uid = id;
   

    if (uid==ROOM_ACTION) {
        //Fonksiyondan gelen Aksiyon CPU1 e gonderiliyor
        cJSON *root = cJSON_CreateObject();
        cJSON *child = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "event");
        if (func->genel.registered)
            cJSON_AddNumberToObject(root, "id", func->genel.register_id);
        else    
            cJSON_AddNumberToObject(root, "id", func->genel.device_id);

        func->get_status_json(child);
        cJSON_AddItemToObject(root, "durum", child);   
        char *dat = cJSON_PrintUnformatted(root);


        while(rs485.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
        return_type_t pp = rs485.Sender(dat,254);
        if (pp!=RET_OK) printf("PAKET GÖNDERİLEMEDİ. Error:%d\n",pp);

        vTaskDelay(50/portTICK_PERIOD_MS); 
        cJSON_free(dat);
        cJSON_Delete(root);    
    }
}

void rs485_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    //ESP_LOGW(TAG, "LED %ld %ld", id , base, id);
    rs485_events_data_t* event = (rs485_events_data_t*) event_data;
    bool broadcast=false;
    if (id==RS485_EVENTS_BROADCAST) broadcast=true;//printf("broadcast\n");;
   // if (id==RS485_EVENTS_DATA) printf("data %s %d %d\n",event->data, event->data_len, event->sender);

   //printf("%s\n",event->data);

    cJSON *rcv = cJSON_Parse(event->data);
    if (rcv==NULL) return; 
    char *command = (char *)calloc(1,20); 
    JSON_getstring(rcv,"com", command,19); 
    
        /*
        CLIENT COMMAND 
        Açılışta broadcast olarak sinfo gönderdik. Karşılığında server
        kendini tanıtan bir sinfo_ack gönderdi. S_INFO noktasını serbest
        bırak.
        */
        if (strcmp(command,"sinfo_ack")==0) { 
            if (mainbox_ok!=NULL) xSemaphoreGive(mainbox_ok);            
        }

    /*
       CLIENT COMMAND 
       Register olmak için cihaz R_REQ göndermiştir. Ana cihaz Register 
       işlemini tamamlamış ve R_ACK göndermiştir. Fonksiyona register oldugunu 
       söylememiz ve idsini değiştirmesini sağlamamız gerekiyor.
    */
      if (strcmp(command,"R_ACK")==0)
      {
        uint8_t id0 = 0, id1 = 0;
        JSON_getint(rcv,"req_id",&id0);
        JSON_getint(rcv,"ack_id",&id1);
        Base_Function *aa = function_find(id0);
        if (aa!=NULL) {
                ESP_LOGE(TAG, "%d -> %d registered",id0,id1);
                aa->function_set_register(id1,event->sender);
                function_reg_t rr = {};
                rr.device_id =aa->genel.device_id;
                rr.register_id = aa->genel.register_id;
                rr.register_device_id = aa->genel.register_device_id;
                strcpy(rr.name,aa->genel.name);
                strcpy(rr.auname,aa->genel.uname);
                rr.prg_loc = 0;
                disk.write_file(FUNCTION_FILE,&rr,sizeof(rr),rr.device_id);
                if (register_ready!=NULL) xSemaphoreGive(register_ready);           
                      } 
      }

    if (strcmp(command,"event")==0)
    {
        uint8_t iid = 0;
        if (JSON_getint(rcv,"id",&iid))
        {
            //Event register olmuş bir fonksiyondan geliyor. 
            //event olarak yayınlanacak
            home_status_t ss = {};
            home_status_t old = {};

            //printf("id = %d sender %d\n",iid,event->sender);;
            Base_Function *aa = find_register(event->sender,iid);
            if (aa==NULL) aa =function_find(iid);
            if (aa!=NULL)
              {
                //printf("Bulundu %s\n",aa->genel.name);
                old = aa->get_status();
                json_to_status(rcv,&ss,old);
                ss.id = iid;
                //printf("EVENT YAYINI %d\n",iid);
                ESP_ERROR_CHECK(esp_event_post(FUNCTION_IN_EVENTS, FUNCTION_ACTION, (void*)&ss, sizeof(home_status_t), portMAX_DELAY));
              }
            free(command);  
            cJSON_Delete(rcv);  
            return ;      
        }

        char *nm = (char *)calloc(1,20); 
        JSON_getstring(rcv,"name", nm,19);  
        if (strncmp(nm,"AN",2)==0)
        {
            home_virtual_t ss = {};
            strcpy((char*)ss.name,nm);
            uint8_t st=0;
            JSON_getint(rcv,"stat",&st);
            ss.stat = st;
            ss.sender = event->sender;
            ESP_ERROR_CHECK(esp_event_post(VIRTUAL_EVENTS, VIRTUAL_DATA, (void*)&ss, sizeof(home_virtual_t), portMAX_DELAY));
        }
    }


    free(command);  
    cJSON_Delete(rcv); 
}