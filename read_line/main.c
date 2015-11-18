/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * main.c
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

struct buffer
{
  size_t length, size;
  char* data;
};

int read_line(FILE* fp, struct buffer* buf)
{
  char c;
  if (fp == NULL) {
    free(buf->data);
    buf->data = NULL;
    buf->size = 0;
    buf->length = 0;
    return 0;
  }

  buf->length = 0;
  memset(buf->data, 0, buf->size);
  while((c = getc(fp)) != EOF) {
    if (buf->size < buf->length + 2) {  // Including a null terminator
      char* new_data = NULL;
      size_t new_size = buf->size * 2;
      if (new_size == 0)
        new_size = 1024;
      new_data = realloc(buf->data, new_size);
      if (new_data == NULL)
        return 1;  // Memory error
      buf->size = new_size;
      buf->data = new_data;
    }
    buf->data[buf->length++] = c;
    if (c == '\n')
      break;
  }
  buf->data[buf->length] = '\0';
  if (buf->length == 0 && c == EOF)
    return 2;
  return 0;
}


int main()
{
  struct buffer buf = {0, 0, NULL};
  while (read_line(stdin, &buf) == 0) {
    printf("> %s", buf.data);
  }
  read_line(NULL, &buf);
  return 0;
}
