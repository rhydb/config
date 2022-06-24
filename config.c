#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static int
parseline(char *line, int len, char *key, char *val)
{
  int si = 0;
  int ki;
  int vi;
  char quotes = 0;

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
  for (; si < len && line[si] == ' ' || line[si] == RB_CONFIG_DELIM; si++);
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
  int i;
  size_t len;
  char *line = NULL;
  char key[RB_CONFIG_MAX_KEY_LEN];
  char val[RB_CONFIG_MAX_VAL_LEN];

  while ((len = getline(&line, &len, in)) != -1) {
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
          sscanf(val, cfg[i].fmt, cfg[i].ptr);
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

    strcpy(line + offset, " = ");
    offset += 3; // strlen(" = ")

    if (strcmp(cfg[i].fmt, "%s") == 0)
      strcpy(line+offset, cfg[i].ptr);
    else
      sprintf(line + offset, cfg[i].fmt, *((char*)(cfg[i].ptr)));

    fputs(line, out);
    fputc('\n', out);
  }
}

