/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * read_stdin/main.c
 */

#include <stdio.h>
#include <stdlib.h>

struct buffer
{
  size_t size, length;
  char* data;
};

int buffer_resize(struct buffer* buf, size_t size)
{
  char* new_data = realloc(buf->data, size);
  if (new_data) {
    buf->size = size;
    buf->data = new_data;
    return 0;
  }
  return 1;
}

void buffer_free(struct buffer* buf)
{
  free(buf->data);
  buf->data = NULL;
  buf->size = 0;
  buf->length = 0;
}

int main()
{
  struct buffer buf = {0, 0, NULL};
  while (!feof(stdin)) {
    size_t delta = 0;
    if (buffer_resize(&buf, buf.size + BUFSIZ) != 0) {
      fprintf(stderr, "memory error allocating %zu bytes\n", buf.length + BUFSIZ);
      buffer_free(&buf);
      return EXIT_FAILURE;
    }
    delta = buf.size - buf.length;
    buf.length += fread(buf.data + buf.length, 1, delta - 1, stdin);
  }
  buf.data[buf.length] = '\0';
  printf("length: %zu size: %zu\n", buf.length, buf.size);
  return 0;
}
