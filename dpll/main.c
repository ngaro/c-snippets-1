/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * dpll/main.c
 */

#include "dpll.h"
#include <stdlib.h>
#include <time.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool sat_callback(size_t num_vars, bool* values, void* userdata)
{
  size_t index;
  fputs("SAT\n", stdout);
  for (index = 0; index < num_vars; ++index) {
    if (values[index])
      printf(" %d\n", index + 1);
    else
      printf("-%d\n", index + 1);
  }
  printf("\n");
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int main()
{
  int res;
  size_t index;
  bool* values = NULL;
  struct clause_set set;
  clock_t tstart = 0, tdelta = 0;

  clause_set_init(&set);

  tstart = clock();
  if (!clause_set_parse(&set, stdin)) {
    printf("error: %s\n", dpll_errinfo);
    return EXIT_FAILURE;
  }
  tdelta = clock() - tstart;
  fprintf(stderr, "parsing: %f seconds\n", (double) tdelta / CLOCKS_PER_SEC);

  tstart = clock();
  if (!clause_set_solve(&set, &values, sat_callback, NULL)) {
    fputs("UNSAT\n", stdout);
    return EXIT_SUCCESS;
  }
  tdelta = clock() - tstart;
  fprintf(stdout, "solving: %f seconds\n", (double) tdelta / CLOCKS_PER_SEC);

  free(values);
  values = NULL;
  return 0;
}
