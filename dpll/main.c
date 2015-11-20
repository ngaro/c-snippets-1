/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * dpll/main.c
 */

#include "dpll.h"
#include <stdlib.h>
#include <errno.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int main()
{

  int res;
  size_t index;
  bool* values = NULL;
  struct clause_set set;
  clause_set_init(&set);
  if (!clause_set_parse(&set, stdin)) {
    printf("error: %s\n", dpll_errinfo);
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
      printf(" %d\n", index + 1);
    else
      printf("-%d\n", index + 1);
  }
  return 0;
}
