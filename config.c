#include "config.h"
#include "getline.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int
parseline(char *line, int len, char *key, char *val)
{
  int si = 0;
  int ki;
  int vi;

  /* skip spaces until key */
  for (si = 0; si < len && line[si] == ' '; si++);
  if (si == len)
    return 1;
  /* read key until space or delim */
  for (ki = 0; si < len && line[si] != ' ' && line[si] != RB_CONFIG_DELIM; si++) {
    if (ki == RB_CONFIG_MAX_KEY_LEN) {
      fprintf(stderr, "Key '%s' reached MAX_KEY_LEN (%d)\n", key, RB_CONFIG_MAX_KEY_LEN);
      return 1;
    }
    key[ki++] = line[si];
  }
  key[ki] = '\0';

  if (ki == 0) {
    fprintf(stderr, "Could not find key for line '%s'\n", line);
    return 1;
  }

  /* skip until value */
  for (; (si < len) && (line[si] == ' ' || line[si] == RB_CONFIG_DELIM); si++);
  if (si == len) {
    fprintf(stderr, "key '%s' does not have matching value\n", key);
    return 1;
  }

  /* read value until end */
  for (vi = 0; si < len && vi < RB_CONFIG_MAX_VAL_LEN; si++) {
    val[vi++] = line[si];
  }
  if (vi == RB_CONFIG_MAX_KEY_LEN) {
    fprintf(stderr, "Value '%s' reached MAX_VAL_LEN (%d)\n", val, RB_CONFIG_MAX_VAL_LEN);
    return 1;
  }
  val[vi] = '\0';
  return 0;
}

void
rb_readcfg(FILE *in, cfg_t cfg[], size_t n)
{
  size_t i;
  size_t len;
  char *line = NULL;
  char key[RB_CONFIG_MAX_KEY_LEN];
  char val[RB_CONFIG_MAX_VAL_LEN];
  char fmt[6];

  while ((len = rb_getline(&line, &len, in)) != -1) {
    if (len > 1) {
      /* line[--len] = '\0'; */
      if (line[len-1] == '\n')
        line[--len] = '\0';

      if (parseline(line, len, key, val)) {
        fprintf(stderr, "Failed to parse line\n");
        continue;
      }
      for (i = 0; i < n; i++) {
        if (strcmp(cfg[i].key, key) == 0) {
          if (strcmp(cfg[i].fmt, "%s") == 0) {
            strcpy(cfg[i].ptr, val);
            break;
          }

          size_t fmt_len = strlen(cfg[i].fmt);
          char pos = 0;
          fmt[pos++] = '%';

          if (fmt_len > 2) {
            if (cfg[i].fmt[fmt_len - 2] == 'h' || cfg[i].fmt[fmt_len - 2] == 'l') {
              fmt[pos++] = cfg[i].fmt[fmt_len - 2]; // length
              if (fmt_len > 3) {
                if (cfg[i].fmt[fmt_len - 3] == cfg[i].fmt[fmt_len - 2]) {
                  fmt[pos++] = cfg[i].fmt[fmt_len - 3]; // optional second length, e.g '%lld'
                }
              }
            }
          }
          fmt[pos++] = cfg[i].fmt[fmt_len - 1]; // type
          fmt[pos++] = '\0';

          sscanf(val, fmt, cfg[i].ptr);
          break;
        }
      }
      if (i == n) {
        fprintf(stderr, "Unknown key '%s'\n", key);
      }
    }
  }

  free(line);
}

void
rb_writecfg(FILE *out, cfg_t cfg[], size_t n)
{
  size_t i;
  /* 'key = val' */
  char line[RB_CONFIG_MAX_KEY_LEN + 3 + RB_CONFIG_MAX_VAL_LEN];
  for (i = 0; i < n; i++) {
    unsigned int offset;

    strcpy(line, cfg[i].key);
    offset = strlen(cfg[i].key);

    line[offset++] = ' ';
    line[offset++] = RB_CONFIG_DELIM;
    line[offset++] = ' ';

#define CAST_TYPE(type) (*((type *)(cfg[i].ptr) ))
    size_t len = strlen(cfg[i].fmt);
    char last_char = cfg[i].fmt[len - 1];
    char data_length = 0;
    if (len > 2) { // %_x
      if (cfg[i].fmt[len - 2] == 'l' || cfg[i].fmt[len - 2] == 'h') {
        data_length = 1;
        if (len > 3) { // %llx
          if (cfg[i].fmt[len - 3] == cfg[i].fmt[len - 2]) {
            data_length = 2;
          }
        }
      }
    }
    switch (last_char) {
      case 's':
        sprintf(line + offset, cfg[i].fmt, CAST_TYPE(char*));
        break;
      case 'c':
        sprintf(line + offset, cfg[i].fmt, CAST_TYPE(char));
        break;
      case 'p':
        sprintf(line + offset, cfg[i].fmt, CAST_TYPE(void*));
        break;
      case 'd':
      case 'i':
        if (data_length > 0) {
          if (cfg[i].fmt[len - 2] == 'l') {
            if (data_length > 1) {
              // %lld
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(long long));
            } else {
              // %ld
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(long));
            }
          } else  {
            if (data_length > 1) {
              // %hhd
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(char));
            } else {
              // %hd
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(short));
            }
          }
        } else {
          sprintf(line + offset, cfg[i].fmt, CAST_TYPE(int));
        }
        break;
      case 'u':
      case 'x':
      case 'X':
      case 'o':
        if (data_length > 0) {
          if (cfg[i].fmt[len - 2] == 'l') {
            if (data_length > 1) {
              // %llu
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(unsigned long long));
            } else {
              // %lu
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(unsigned long));
            }
          } else  {
            if (data_length > 1) {
              // %hhu
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(unsigned char));
            } else {
              // %hu
              sprintf(line + offset, cfg[i].fmt, CAST_TYPE(unsigned short));
            }
          }
        } else {
          sprintf(line + offset, cfg[i].fmt, CAST_TYPE(unsigned int));
        }
        sprintf(line + offset, cfg[i].fmt, CAST_TYPE(unsigned int));
        break;
      case 'f':
      case 'F':
      case 'e':
      case 'E':
      case 'g':
      case 'G':
      case 'a':
      case 'A':
        if (data_length > 0) {
          sprintf(line + offset, cfg[i].fmt, CAST_TYPE(double));
        } else {
          sprintf(line + offset, cfg[i].fmt, CAST_TYPE(float));
        }
        break;
      default:
        sprintf(line + offset, cfg[i].fmt, CAST_TYPE(char));
    }

    fputs(line, out);
    fputc('\n', out);
  }
}

