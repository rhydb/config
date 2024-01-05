#ifndef RB_CONFIG_H
#define RB_CONFIG_H

#ifndef RB_CONFIG_MAX_KEY_LEN
  #define RB_CONFIG_MAX_KEY_LEN 50
#endif

#ifndef RB_CONFIG_MAX_VAL_LEN
  #define RB_CONFIG_MAX_VAL_LEN 50
#endif

#ifndef RB_CONFIG_DELIM
  #define RB_CONFIG_DELIM '='
#endif

#include <stdio.h>
#include <stddef.h>

typedef struct cfg {
  const char *key;
  const char *fmt;
  void *ptr;
  const char *def;
} cfg_t;

void rb_readcfg(FILE *in, cfg_t cfg[], size_t n);
void rb_writecfg(FILE *out, cfg_t cfg[], size_t n);
void rb_writecfg_def(FILE *out, cfg_t cfg[], size_t n);
void rb_defaultcfg(cfg_t cfg[], size_t n);

#endif // RB_CONFIG_H

