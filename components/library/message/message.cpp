
#include "message.h"
#include "esp_log.h"

static const char *MESSAGE_TAG = "MESSAGE";

//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void Message::set_status(home_status_t stat)
{      
}


//statusu json olarak döndürür
void Message::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
}

void Message::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}

void Message::mesaj_yayinla(void)
{
    ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, MESSAGE, this, sizeof(Message), portMAX_DELAY));
}

void Message::json_mes(const char *ms)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", "message");
    cJSON_AddStringToObject(root, "text", ms);  
    char *dat = cJSON_PrintUnformatted(root);
    strcpy(message,dat);
    cJSON_free(dat);
    cJSON_Delete(root);  
    mesaj_yayinla(); 
}


//Yazılım ile gelen eventler
void Message::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_message_t *st = (home_message_t *) event_data;
    Message *mes = (Message *) handler_args;
    if (id==MOVEMEND) return;
    
    if (id==MESSAGE_CLNOK)
    {
        ESP_LOGI(MESSAGE_TAG,"CleanOK mesajı");
        mes->json_mes("Odaniz Temizlendi");
    }
    if (id==MESSAGE_EMERGENCY)
    {
        ESP_LOGI(MESSAGE_TAG,"Emergency mesajı");
        mes->json_mes("Acil durum bildirimi yaptiniz");
    }
    if (id==MESSAGE_FIRE)
    {
        ESP_LOGI(MESSAGE_TAG,"Yangin mesajı");
        mes->json_mes("Yangin Algilandi");
    }
    if (id==MESSAGE_ALARM)
    {
        ESP_LOGI(MESSAGE_TAG,"Alarm mesajı");
        mes->json_mes("Alarm Algilandi");
    }

    if (id==MESSAGE_IN ) {
        if (strlen(st->txt)>2)
        {
        ESP_LOGI(MESSAGE_TAG,"Resepsiyon mesajı");
        cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "message");
        cJSON_AddStringToObject(root, "text", st->txt);  
        char *dat = cJSON_PrintUnformatted(root);
        strcpy(mes->message,dat);
        cJSON_free(dat);
        cJSON_Delete(root);  
        mes->mesaj_yayinla();
        } else {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "message");
            cJSON_AddStringToObject(root, "text", " ");   
            char *dat = cJSON_PrintUnformatted(root);
            strcpy(mes->message,dat);
            cJSON_free(dat);
            cJSON_Delete(root);  
            mes->mesaj_yayinla(); 
        }
    }

    if (id==MESSAGE ) {
        if (strlen(st->txt)>2)
        {
            ESP_LOGI(MESSAGE_TAG,"Sistem check in mesajı");
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "message");
            char *ff = (char*)calloc(1,300);
            strcpy(ff,"Hi ");
            strcat(ff,st->txt);
            strcat(ff,", welcome to our hotel. We wish you a nice holiday.");
            cJSON_AddStringToObject(root, "text", ff);  
            char *dat = cJSON_PrintUnformatted(root);
            strcpy(mes->message,dat);
            cJSON_free(dat);
            cJSON_Delete(root);  
            mes->mesaj_yayinla();
        } else {
            cJSON *root = cJSON_CreateObject();
            cJSON_AddStringToObject(root, "com", "message");
            cJSON_AddStringToObject(root, "text", " ");   
            char *dat = cJSON_PrintUnformatted(root);
            strcpy(mes->message,dat);
            cJSON_free(dat);
            cJSON_Delete(root);  
            mes->mesaj_yayinla(); 
        }
    }

}

void Message::init(void)
{
    if (!genel.virtual_device)
    {
        ESP_ERROR_CHECK(esp_event_handler_instance_register(MESSAGE_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
      //  ESP_ERROR_CHECK(esp_event_handler_instance_register(VIRTUAL_EVENTS, ESP_EVENT_ANY_ID, virtual_handler, (void *)this, NULL));
        set_status(status);
    }
}

/*
cJSON *root = cJSON_CreateObject();
        cJSON_AddStringToObject(root, "com", "message");
        char * qq = (char*)calloc(1,100);
        strcpy(qq,"Hi ");
        strcat(qq,((Checkin*)event_data)->get_misafir());
        strcat(qq,", welcome to our hotel. We wish you a nice holiday.");
        cJSON_AddStringToObject(root, "text", qq);  
        char *dat = cJSON_PrintUnformatted(root);
        while(uart.is_busy()) vTaskDelay(50/portTICK_PERIOD_MS);
        return_type_t pp = uart.Sender(dat,253);
        if (pp!=RET_OK) printf("PAKET GÖNDERİLEMEDİ. Error:%d\n",pp);
        vTaskDelay(50/portTICK_PERIOD_MS); 
        cJSON_free(dat);
        cJSON_Delete(root);  
        free(qq); 
*/

/*
    Mesajları takip için  kullanılan bir objedir.
    olası Tanım :

    {
            "name":"message",
            "uname":"Mesajlar",
            "id": 5,
            "loc": 1,
            "hardware": {
                "location" : "local",
                "port": []
            }
        }

    

  
*/

