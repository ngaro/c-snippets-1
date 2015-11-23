/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * endianess/main.c
 */

#include <stdio.h>
#include <stdint.h>

int main()
{
  union {
    uint32_t x;
    uint8_t b[4];
  } e = { 1 };
  if (e.b[0] == 1) {
    puts("little");
  }
  else {
    puts("big");
  }
  return 0;
}

