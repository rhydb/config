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
  for (ki = 0; si < len && line[si] != ' ' && line[si] != DELIM; si++) {
    if (ki == MAX_KEY_LEN) {
      fprintf(stderr, "Key '%s' reached MAX_KEY_LEN (%d)\n", key, MAX_KEY_LEN);
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
  for (; si < len && line[si] == ' ' || line[si] == DELIM; si++);
  if (si == len) {
    printf("key '%s' does not have matching value\n", key);
    return 1;
  }

  /* read value until end */
  for (vi = 0; si < len; si++) {
    val[vi++] = line[si];
  }

  if (vi == MAX_KEY_LEN) {
    fprintf(stderr, "Value '%s' reached MAX_VAL_LEN (%d)\n", val, MAX_VAL_LEN);
    return 1;
  }
  val[vi] = '\0';
  return 0;
}

void
rb_readcfg(FILE *in, cfg_t cfg[], size_t n)
{
  int i;
  int len;
  char *line = NULL;
  char key[MAX_KEY_LEN];
  char val[MAX_VAL_LEN];

  while ((len = getline(&line, &len, in)) != -1) {
    if (len > 1) {
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

