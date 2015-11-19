/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * dpll/main.c
 */

#include "dpll.h"
#include <stdlib.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int main()
{
  int res;
  size_t index;
  bool* values = NULL;
  struct clause_set set;
  clause_set_init(&set);
  res = clause_set_parse(&set, stdin);
  switch (res) {
    case 0:
      break;
    case -1:
      fputs("memory error\n", stderr);
      return EXIT_FAILURE;
    case 1:
    default:
      fputs("invalid input format\n", stderr);
      return EXIT_FAILURE;
  }

  res = clause_set_solve(&set, &values);
  if (!res) {
    fputs("UNSAT\n", stdout);
    return EXIT_SUCCESS;
  }

  fputs("SAT\n", stdout);
  for (index = 0; index < set.num_vars; ++index) {
    if (values[index])
      printf(" %d\n", index);
    else
      printf("-%d\n", index);
  }
  return 0;
}