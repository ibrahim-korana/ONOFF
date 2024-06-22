#ifndef _STORAGE_H
#define _STORAGE_H

#include "core.h"
#include "esp_spiffs.h"

#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include "esp_err.h"
#include "esp_log.h"
#include <errno.h>
#include <sys/time.h>
#include "esp_vfs.h"
#include <sys/fcntl.h>
#include <unistd.h>
#include <ctype.h>

#define FORMAT_SPIFFS_IF_FAILED true

#define STATUS_FILE "/config/status.bin"
#define FUNCTION_FILE "/config/function.bin"

class Storage {
    public:
      Storage() {};
      ~Storage(){};

      esp_err_t init(void);
      bool format(void);

      bool file_search(const char *name);
      bool file_empty(const char *name);
      bool file_control(const char *name);
      int file_size(const char *name);
      bool file_create(const char *name, uint16_t size);
      
      bool write_file(const char *name, void *flg, uint16_t size, uint8_t obj_num);
      bool read_file(const char *name, void *flg, uint16_t size, uint8_t obj_num);
      
      bool file_format(const char *fn, uint16_t size);
      bool function_file_format(void);
      bool status_file_format(void);
      bool write_status(home_status_t *stat, uint8_t obj_num);
      bool read_status(home_status_t *stat, uint8_t obj_num);


      static const char *rangematch(const char *pattern, char test, int flags);
      static int fnmatch(const char *pattern, const char *string, int flags);
      static void list(const char *path, const char *match);

    protected:
     FILE * dosyaac(const char *fn, const char *mode);
      void dosyakapat(FILE *ff);

};

#endif
