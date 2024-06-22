

#include "cJSON.h"
#include "string.h"
#include "core.h"

bool JSON_item_control(cJSON *rt,const char *item)
{
    if (cJSON_GetObjectItem(rt, item)) return true;
    else return false;
}

bool JSON_getstring(cJSON *rt,const char *item, char *value, uint8_t len)
{
    if (JSON_item_control(rt, item)) {
              char *tmp = (char *)cJSON_GetObjectItem(rt,item)->valuestring;
              if (strlen(tmp)>len) return false;
              strcpy(value , tmp);
              return true; 
	                                 } 
    else return false;
}

bool JSON_getint(cJSON *rt,const char *item, uint8_t *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valueint;
        return true;
    }
    return false;
}

bool JSON_getbool(cJSON *rt,const char *item, bool *value)
{
    if (JSON_item_control(rt, item)) {
        *value = (bool *)cJSON_GetObjectItem(rt,item)->valueint;
        return true;
    }
    return false;
}

bool JSON_getfloat(cJSON *rt,const char *item, float *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valuedouble;
        return true;
    }
    return false;
}

bool JSON_get64long(cJSON *rt,const char *item, uint64_t *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valuedouble;
        return true;
    }
    return false;
}

bool JSON_getlong(cJSON *rt,const char *item, uint32_t *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valuedouble;
        return true;
    }
    return false;
}

bool JSON_get16long(cJSON *rt,const char *item, uint16_t *value)
{
    if (JSON_item_control(rt, item)) {
        *value = cJSON_GetObjectItem(rt,item)->valueint;
        return true;
    }
    return false;
}

void json_to_status(cJSON *rt, home_status_t *stat, home_status_t old)
{
   if (JSON_item_control(rt,"durum")) 
        {
            cJSON *fmt = cJSON_GetObjectItem(rt,"durum"); 

            /*          
            if (JSON_item_control(fmt,"stat")) stat->stat = cJSON_GetObjectItem(fmt,"stat")->valueint;
            if (JSON_item_control(fmt,"temp")) stat->temp = cJSON_GetObjectItem(fmt,"temp")->valuedouble;
            if (JSON_item_control(fmt,"stemp")) stat->set_temp = cJSON_GetObjectItem(fmt,"stemp")->valuedouble;
            if (JSON_item_control(fmt,"color")) stat->color = (uint64_t)cJSON_GetObjectItem(fmt,"color")->valuedouble; 
            if (JSON_item_control(fmt,"status")) stat->status = cJSON_GetObjectItem(fmt,"status")->valueint;
            if (JSON_item_control(fmt,"ircom")) strcpy((char*)stat->ircom,cJSON_GetObjectItem(fmt,"ircom")->valuestring);
            if (JSON_item_control(fmt,"irval")) strcpy((char*)stat->irval,cJSON_GetObjectItem(fmt,"irval")->valuestring);
            if (JSON_item_control(fmt,"coun")) stat->counter = cJSON_GetObjectItem(fmt,"coun")->valueint;
            if (JSON_item_control(fmt,"act")) stat->active = cJSON_GetObjectItem(fmt,"act")->valueint;
            */
            if (!JSON_getbool(fmt,"stat",&stat->stat)) stat->stat=old.stat;
            if (!JSON_getfloat(fmt,"temp",&stat->temp)) stat->temp = old.temp;
            if (!JSON_getfloat(fmt,"stemp",&stat->set_temp)) stat->set_temp = old.set_temp;
            
            bool ff = JSON_get64long(fmt,"color",&stat->color);

            if (!ff) stat->color = old.color;
            
          //  printf("ff = %d COLOR CEVIRI=%ld\n",ff,stat->color);
            if (!JSON_getint(fmt,"status",&stat->status)) stat->status=old.status;
            if (!JSON_getstring(fmt,"ircom",(char *)&(stat->ircom),31)) strcpy((char*)stat->ircom,(char*)old.ircom);
            if (!JSON_getstring(fmt,"irval",(char *)&(stat->irval),31)) strcpy((char*)stat->irval,(char*)old.irval);
            if (!JSON_getint(fmt,"coun",&stat->counter)) stat->counter = old.counter;
            if (!JSON_getbool(fmt,"act",&stat->active)) stat->active = old.active;
        }
}


char *mem_res(uint8_t len)
  {
	  char *mem0 = (char*)malloc(len);memset(mem0,0,len);
	  return mem0;
  };

