#ifndef _JSON_TOOL_H_
#define _JSON_TOOL_H_

  #include "cJSON.h"
  #include "lwip/err.h"
  #include "lwip/sys.h"
  #include "lwip/etharp.h"
  #include "core.h"

  bool JSON_item_control(cJSON *rt,const char *item);
  bool JSON_getstring(cJSON *rt,const char *item, char *value, uint8_t len);
  bool JSON_getint(cJSON *rt,const char *item, uint8_t *value);
  bool JSON_getbool(cJSON *rt,const char *item, bool *value);
  bool JSON_getfloat(cJSON *rt,const char *item, float *value);
  bool JSON_get64long(cJSON *rt,const char *item, uint64_t *value);
  bool JSON_getlong(cJSON *rt,const char *item, uint32_t *value);
  bool JSON_get16long(cJSON *rt,const char *item, uint16_t *value);

  char *mem_res(uint8_t len);

  void json_to_status(cJSON *rt, home_status_t *stat, home_status_t old);

#endif
