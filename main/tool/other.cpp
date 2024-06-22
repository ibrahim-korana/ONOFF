
void info(const char*komut, char* data);

void mbready_task(void *arg)
{
    //sinfo komutu gönderiliyor.. Cevaben sinfo_ack unicast üzerinden gelecek.
    vTaskDelay((GlobalConfig.device_id * 100)/portTICK_PERIOD_MS);
    ESP_LOGW(TAG,"Ana Cihaz Kontrol Ediliyor.." );
    char *dt = (char*)malloc(200);
    info("sinfo",dt);
    Global_Send(dt,255,TR_SERIAL);
    bool ck = true;
    uint8_t count = 0;
    while (ck)
    {
        if (xSemaphoreTake(mainbox_ok, (3000/portTICK_PERIOD_MS))==pdTRUE)
        {
            SERVER_READY = true;
            xSemaphoreGive(mainbox_ready);
            ck=false;
        } else {
            Global_Send(dt,255,TR_SERIAL);
            if(++count>20) {
                SERVER_READY = false;
                ck=false;
                xSemaphoreGive(mainbox_ready);
            }
        }
    }
    free(dt);
    vTaskDelete(NULL);
}

void info(const char*komut, char* data)
{
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "com", komut);
    cJSON_AddNumberToObject(root, "cid", GlobalConfig.device_id);  
   // cJSON_AddStringToObject(root, "mac", (char*)NetworkConfig.mac);
   // cJSON_AddStringToObject(root, "ip", Addr.to_string(NetworkConfig.home_ip));
    cJSON_AddNumberToObject(root, "fcount", function_count());  
    char *dat = cJSON_PrintUnformatted(root);
    strcpy(data,dat);
    cJSON_free(dat);
    cJSON_Delete(root); 
}

static void Register_Callback(void *arg)
{
  Base_Function *mthis = (Base_Function *)arg;

  cJSON *root = cJSON_CreateObject();
  cJSON *child = cJSON_CreateObject();

  //printf("%d Register\n",mthis->genel.device_id);

  cJSON_AddStringToObject(root, "com", "R_REQ");
  cJSON_AddNumberToObject(root, "req_id", mthis->genel.device_id);
  cJSON_AddStringToObject(root, "name", mthis->genel.name);
  char *mm = (char *)calloc(1,15);
  sprintf(mm,"AU%02d%02d",GlobalConfig.device_id,mthis->genel.device_id);
  strcpy(mthis->genel.uname,mm);
  cJSON_AddStringToObject(root, "auname", mm);
  free(mm);
  cJSON_AddNumberToObject(root, "loc", mthis->location);
    mthis->get_status_json(child);
    cJSON_AddItemToObject(root, "durum", child); 
    mthis->get_port_json(root);  
  char *dat = cJSON_PrintUnformatted(root);
  //Sender(dat,RS485_MASTER);
  Global_Send(dat,RS485_MASTER,TR_SERIAL);
  vTaskDelay(50/portTICK_PERIOD_MS);
  cJSON_free(dat);
  cJSON_Delete(root); 
}

void register_task(void *arg)
{
    //Register olmayan fonksiyonlar register edilecek.
    Base_Function *target = get_function_head_handle();
    register_ready = xSemaphoreCreateBinary();
    assert(register_ready);
    uint8_t vcount = 0;
    while(target)
    {
        if (target->genel.register_id==0 && target->genel.register_device_id==0)
        {
            Register_Callback(target);
            if (xSemaphoreTake(register_ready, 1500/portTICK_PERIOD_MS)==pdTRUE)
            {
                target = (Base_Function *) target->next;
                vcount = 0;
            } else if (++vcount>4) target=NULL;
        } else target = (Base_Function *) target->next;        
    }
    xSemaphoreGive(mainbox_ready);
    vSemaphoreDelete(register_ready);
    register_ready=NULL;
    vTaskDelete(NULL);
}


void response_task(void *arg)
{
  response_par_t *par = (response_par_t *)arg;
  char *mm=(char*)calloc(1,strlen(par->data)+1);
  strcpy(mm,par->data);
  Global_Send(mm,par->sender,par->trn);
  free(mm);
  vTaskDelete(NULL);
}


void function_register_all(void *arg)
{
  mainbox_ready = xSemaphoreCreateBinary();
  assert(mainbox_ready);

    ESP_LOGW(TAG,"Register kayitlari siliniyor");
    function_reg_t uv = {};
    for (int i=0;i<MAX_DEVICE;i++) disk.write_file(FUNCTION_FILE,&uv,sizeof(uv),i);
    Base_Function *target = get_function_head_handle();
    while(target)
    {
        target->genel.register_id=0;
        target->genel.register_device_id=0;
        target->genel.registered=0;
        target = (Base_Function *) target->next;        
    }


  xTaskCreate(register_task, "mb_02", 2048, NULL, 10, NULL);
  xSemaphoreTake(mainbox_ready, portMAX_DELAY);
  vSemaphoreDelete(mainbox_ready);
  mainbox_ready=NULL;
  ESP_LOGW(TAG,"Register işlemi tamamlandı");

  cJSON* root = cJSON_CreateObject();
  cJSON_AddStringToObject(root, "com", "R_REG_OK");   
  char *dat = cJSON_PrintUnformatted(root);  
            response_par_t rr = {};
            rr.data = dat;
            rr.sender = RS485_MASTER;
            rr.trn = TR_SERIAL;
            xTaskCreate(response_task,"rtask",2048,&rr,5,NULL);
            vTaskDelay(10/portTICK_PERIOD_MS); 
  cJSON_free(dat);
  cJSON_Delete(root); 
  function_list(true); 

  vTaskDelete(NULL);
}
