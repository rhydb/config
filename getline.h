#pragma once

#include <stdlib.h>

ssize_t
rb_getline(char **lineptr, size_t *n, FILE *stream)
{
  const size_t step = sizeof(char) * 100;
  static size_t size;
  static FILE* oldstream = NULL;
  size_t written = 0;
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

  return c == EOF ? -1 : written;
}
