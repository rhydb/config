#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

ssize_t
cfg_getline(char **lineptr, size_t *n, FILE *stream)
{
  const size_t step = sizeof(char) * 10;
  static ssize_t size;
  static FILE* oldstream = NULL;
  ssize_t written = 0;
  char c;

  if (stream != oldstream) {
    size = step;
  }

  if (*lineptr == NULL) {
    *lineptr = malloc(size);
  } else {
    size = *n;
  }

  while ((c = fgetc(stream)) != EOF) {
    if (written + 1 >= size) {
      size += step;
      *lineptr = realloc(*lineptr, size);
    }
    (*lineptr)[written++] = c;
    if (c == '\n') {
      (*lineptr)[written] = '\0';
      break;
    }
  }

  oldstream = stream;
  *n = size;

  return c == EOF ? -1 : written;
}

static void
clean_format(const char *fmt, char *dst)
{
    size_t fmt_len = strlen(fmt);
    unsigned char pos = 0;
    dst[pos++] = '%';

    if (fmt_len > 2) {
        if (fmt[fmt_len - 2] == 'h' || fmt[fmt_len - 2] == 'l') {
            dst[pos++] = fmt[fmt_len - 2]; // length
            if (fmt_len > 3) {
                if (fmt[fmt_len - 3] == fmt[fmt_len - 2]) {
                    dst[pos++] = fmt[fmt_len - 3]; // optional second length, e.g '%lld'
                }
            }
        }
    }
    dst[pos++] = fmt[fmt_len - 1]; // type
    dst[pos++] = '\0';
}

static void
parse_value(const char *val, cfg_t *cfg)
{
    char fmt[6];
    if (strcmp(cfg->fmt, "%s") == 0) {
        strcpy(cfg->ptr, val);
        return;
    }

    clean_format(cfg->fmt, fmt);
    sscanf(val, fmt, cfg->ptr);
}

static void
read_value(const char *str, char *val)
{
    strncpy(val, str, RB_CONFIG_MAX_VAL_LEN);
}

static int
parseline(char *line, int len, char *key, char *val)
{
  int si = 0;
  int ki;

  /* skip spaces until key */
  for (si = 0; si < len && line[si] == ' '; si++);
  if (si == len)
    return 1;
  /* read key until space or delim */
  for (ki = 0; si < len && line[si] != ' ' && line[si] != RB_CONFIG_DELIM; si++) {
    if (ki == RB_CONFIG_MAX_KEY_LEN) {
      fprintf(stderr, "rbconfig: Key '%s' reached MAX_KEY_LEN (%d)\n", key, RB_CONFIG_MAX_KEY_LEN);
      return 1;
    }
    key[ki++] = line[si];
  }
  key[ki] = '\0';

  if (ki == 0) {
    fprintf(stderr, "rbconfig: Could not find key for line '%s'\n", line);
    return 1;
  }

  /* skip until value */
  for (; (si < len) && (line[si] == ' ' || line[si] == RB_CONFIG_DELIM); si++);
  if (si == len) {
    fprintf(stderr, "rbconfig: key '%s' does not have matching value\n", key);
    return 1;
  }

  read_value(line + si, val);
  return 0;
}

void
rb_defaultcfg(cfg_t cfg[], size_t n)
{
    char val[RB_CONFIG_MAX_VAL_LEN];
    for (size_t i = 0; i < n; i++) {
        read_value(cfg[i].def, val);
        parse_value(val, &cfg[i]);
    }
}

void
rb_readcfg(FILE *in, cfg_t cfg[], size_t n)
{
  size_t i;
  ssize_t len;
  size_t alloc;
  char *line = NULL;
  char key[RB_CONFIG_MAX_KEY_LEN];
  char val[RB_CONFIG_MAX_VAL_LEN];

  rb_defaultcfg(cfg, n);

  while ((len = cfg_getline(&line, &alloc, in)) != -1) {
    if (len > 1) {
      /* line[--len] = '\0'; */
      if (line[len-1] == '\n')
        line[--len] = '\0';

      if (line[0] == '#')
        continue;

      if (parseline(line, len, key, val)) {
        fprintf(stderr, "rbconfig: Failed to parse line: '%s'\n", line);
        continue;
      }
      for (i = 0; i < n; i++) {
        if (strcmp(cfg[i].key, key) == 0) {
            parse_value(val, &cfg[i]);
            break;
        }
      }
      if (i == n) {
        fprintf(stderr, "rbconfig: Unknown key '%s'\n", key);
      }
    }
  }

  free(line);
}

static void
readval(const char *val, const cfg_t *cfg)
{
    size_t fmt_len = strlen(cfg->fmt);
    char fmt[6];
    char pos = 0;
    fmt[pos++] = '%';

    // copy length specifiers
    if (fmt_len > 2) {
        if (cfg->fmt[fmt_len - 2] == 'h' || cfg->fmt[fmt_len - 2] == 'l') {
            fmt[pos++] = cfg->fmt[fmt_len - 2]; // length
            if (fmt_len > 3) {
                if (cfg->fmt[fmt_len - 3] == cfg->fmt[fmt_len - 2]) {
                    fmt[pos++] = cfg->fmt[fmt_len - 3]; // optional second length, e.g '%lld'
                }
            }
        }
    }

    fmt[pos++] = cfg->fmt[fmt_len - 1]; // type
    fmt[pos++] = '\0';

    sscanf(val, fmt, cfg->ptr);
}

void
rb_writecfg_def(FILE *out, cfg_t cfg[], size_t n)
{
    for (size_t i = 0; i < n; i++) {
        fputs(cfg[i].key, out);
        fputs(" = ", out);
        fputs(cfg[i].def, out);
        fputc('\n', out);
    }
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
        sprintf(line + offset, cfg[i].fmt, (char*)cfg[i].ptr);
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

