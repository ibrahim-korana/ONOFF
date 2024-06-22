
#include "dayclean.h"
#include "esp_log.h"

static const char *DAYCLEAN_TAG = "DAY_CLEAN";

tm DayClean::getTimeStruct(){
  struct tm timeinfo;
  time_t now;
  time(&now);
  localtime_r(&now, &timeinfo);
  time_t tt = mktime (&timeinfo);  
  struct tm * tn = localtime(&tt);
  return *tn;
}


//bu fonksiyon fonksiyonu yazılım ile tetiklemek için kullanılır.
void DayClean::set_status(home_status_t stat)
{      
    if (!genel.virtual_device)
    { 
        if (stat.active!=genel.active) genel.active = stat.active;  
        if (status.stat != stat.stat)
        {
            bool chg = false;
            if (status.stat && !stat.stat) 
            {
                //Kapatılıyor
                if (genel.active)
                {
                    struct tm zz = getTimeStruct();
                    start_time = zz;
                    t1_conv(stat.irval, &zz);
                    local_set_status(stat,true);      
                    t2_conv(status.irval,&start_time);
                }
                write_status();
                chg=true;
                if (genel.active) ESP_LOGI(DAYCLEAN_TAG,"Günlük Temizlik KAPATILDI");
            }
            if (!status.stat && stat.stat)
            {
                //Aciliyor
                if (genel.active)
                {
                    struct tm zz = getTimeStruct();
                    start_time = zz;
                    t1_conv(stat.ircom, &zz);
                    local_set_status(stat,true);      
                    t2_conv(status.ircom,&start_time);
                    for (int i=0;i<16;i++) {status.irval[i]=0;}
                }
                write_status();
                chg=true;
                if (genel.active) ESP_LOGI(DAYCLEAN_TAG,"Günlük Temizlik ACILDI");
            }
            if (chg)
               ESP_ERROR_CHECK(esp_event_post(FUNCTION_OUT_EVENTS, ROOM_ACTION, (void *)this, sizeof(DayClean), portMAX_DELAY));
        }       
    } else {
        bool chg = local_set_status(stat,true);
        if (chg)
        {
            ESP_LOGI(DAYCLEAN_TAG,"%d Status Changed",genel.device_id);
            ESP_ERROR_CHECK(esp_event_post(FUNCTION_REMOTE_EVENTS, ROOM_ACTION, (void *)this, sizeof(DayClean), portMAX_DELAY));
        }      
    }
}


//statusu json olarak döndürür
void DayClean::ConvertStatus(home_status_t stt, cJSON* obj)
{
    if (stt.stat) cJSON_AddTrueToObject(obj, "stat"); else cJSON_AddFalseToObject(obj, "stat");
    struct tm tr;
    t2_conv(status.ircom,&tr);
    char *mm=(char*)calloc(1,16);
    char *kk=(char*)calloc(1,16);
    sprintf(mm,"%02d",status.ircom[0]);
    strcpy(kk,mm);
    sprintf(mm,"%02d",status.ircom[1]);
    strcat(kk,mm);
    sprintf(mm,"%02d",status.ircom[3]);
    strcat(kk,mm);
    sprintf(mm,"%02d",status.ircom[4]);
    strcat(kk,mm);
    cJSON_AddStringToObject(obj,"ircom",(char*)kk);

    sprintf(mm,"%02d",status.irval[0]);
    strcpy(kk,mm);
    sprintf(mm,"%02d",status.irval[1]);
    strcat(kk,mm);
    sprintf(mm,"%02d",status.irval[3]);
    strcat(kk,mm);
    sprintf(mm,"%02d",status.irval[4]);
    strcat(kk,mm);
    cJSON_AddStringToObject(obj,"irval",(char*)kk);
    if (stt.active) cJSON_AddTrueToObject(obj, "act"); else cJSON_AddFalseToObject(obj, "act");
    cJSON_AddNumberToObject(obj,"oid",genel.oid);
    free(kk);
    free(mm);
}

void DayClean::get_status_json(cJSON* obj) 
{
    return ConvertStatus(status , obj);
}


//Yazılım ile gelen eventler
void DayClean::in_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    home_status_t *st = (home_status_t *) event_data;
    DayClean *con = (DayClean *) handler_args;
    uint8_t dev_id = 0;
    if (id==MOVEMEND) return;
    if (st!=NULL) dev_id = st->id;    
    if (dev_id==con->genel.device_id || id==CLEANOK_ON || id==DAYCLEAN_ON || id==DAYCLEAN_OFF) {
        if (dev_id>0) {
            //Status var bunu kullan
            con->set_status(*st);     
        } else {
            if (id==CLEANOK_ON || id==DAYCLEAN_OFF) {
                home_status_t ss = con->get_status();
                if (ss.stat) {
                    ss.stat = false;
                    con->set_status(ss); 
                }               
                              }
            if (id==DAYCLEAN_ON) {
                home_status_t ss = con->get_status();
                if (!ss.stat) {
                    ss.stat = true;
                    con->set_status(ss); 
                }               
                              }
        }
    } 
}

void DayClean::t1_conv(uint8_t *dest, struct tm *sourge)
{
   dest[0] = sourge->tm_mday;
   dest[1] = sourge->tm_mon;
   dest[2] = sourge->tm_year;
   dest[3] = sourge->tm_hour;
   dest[4] = sourge->tm_min;
   dest[5] = sourge->tm_sec;
}
void DayClean::t2_conv(uint8_t *sourge, struct tm *dest)
{
   dest->tm_mday = sourge[0];
   dest->tm_mon = sourge[1];
   dest->tm_year = sourge[2];
   dest->tm_hour = sourge[3];
   dest->tm_min = sourge[4];
   dest->tm_sec = sourge[5];
}

void DayClean::init(void)
{
    if (!genel.virtual_device)
    {
        t2_conv(status.ircom,&start_time);
        t2_conv(status.irval,&end_time);

        ESP_ERROR_CHECK(esp_event_handler_instance_register(FUNCTION_IN_EVENTS, ESP_EVENT_ANY_ID, in_handler, (void *)this, NULL)); 
        set_status(status);
    }
}


/*
    Günlük Temizliğin tamamlandıgını takip için  kullanılan bir objedir.
    olası Tanım :

    {
            "name":"dayclean",
            "uname":"Günlük Temizlik",
            "id": 5,
            "loc": 1,
            "timer":0,
            "hardware": {
                "location" : "local",
                "port": []
            }
        }

    Açıp kapatma işlemi remote olarak yapılır. 
    saatchange de belirlenen saatte otomatik olarak açılıır. 
    Açılış saati kaydedilir. Açılma anında zaten açıksa yeniden açılmaz. 
    Bu sayede son açılış saati kayıtlı olarak kalır. 
    Kapatma saati de kaydedilir. 
    açma ve kapatma saatleri ircom ve irval üzerinden gönderilir.  

    kapatma işlemi CLEAN_OK ile veya status.stat ile gerçekleşir. 
  
*/

