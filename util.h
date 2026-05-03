#ifndef UTIL_H
#define UTIL_H
#include <stdio.h>

void read_line(char *out, size_t len, FILE *stream) {
  char *str_read = fgets(out, len, stream);
  if (!str_read)
    return;

  int i = 0;
  while (out[i] != '\n' && out[i] != '\0')
    i++;

  if (out[i] == '\n')
    out[i] = '\0';
}

#endif
