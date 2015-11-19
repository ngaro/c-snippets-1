/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * dpll/dpll.c
 */

#include "dpll.h"
#include <assert.h>
#include <stdlib.h>

// The increment in which the capacity of a clause and clause_set
// is incremented.
static size_t const clause_chunksize = 16;
static size_t const clause_set_chunksize = 128;

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void clause_init(struct clause* clause)
{
  clause->count = 0;
  clause->capacity = 0;
  clause->vars = NULL;
  clause->eliminate = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void clause_free(struct clause* clause)
{
  free(clause->vars);
  free(clause->eliminate);
  clause_init(clause);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int clause_add(struct clause* clause, long long const var)
{
  // Check if the variable already exist.
  size_t index;
  for (index = 0; index < clause->count; ++index) {
    if (clause->vars[index] == var)
      return 1;
  }

  // Resize the data if necessary.
  if (clause->count + 1 > clause->capacity) {
    size_t new_capacity = clause->capacity + clause_chunksize;
    long long* new_vars = realloc(clause->vars, sizeof(*new_vars) * new_capacity);
    bool* new_eliminate = realloc(clause->eliminate, sizeof(*new_eliminate) * new_capacity);
    if (new_vars == NULL || new_eliminate == NULL) {
      // Memory error. We free the clause so no invalid frees can happen
      // if a realloc() call succeeded for one of the memory blocks and
      // moved it to another location.
      free(new_vars);
      free(new_eliminate);
      clause_free(clause);
      return 0;
    }
    clause->capacity = new_capacity;
    clause->vars = new_vars;
    clause->eliminate = new_eliminate;
  }

  clause->vars[clause->count] = var;
  clause->eliminate[clause->count] = false;
  clause->count++;
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_shrink_to_fit(struct clause* clause)
{
  assert(clause->count <= clause->capacity);
  if (clause->count < clause->capacity) {
    long long* new_vars = realloc(clause->vars, sizeof(*new_vars) * clause->count);
    bool* new_eliminate = realloc(clause->eliminate, sizeof(*new_eliminate) * clause->count);
    if (new_vars == NULL || new_eliminate == NULL) {
      // Memory error. We free the clause so no invalid frees can happen
      // if a realloc() call succeeded for one of the memory blocks and
      // moved it to another location.
      free(new_vars);
      free(new_eliminate);
      clause_free(clause);
      return false;
    }
    clause->capacity = clause->count;
    clause->vars = new_vars;
    clause->eliminate = new_eliminate;
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_eliminate(struct clause* clause, long long const var, bool eliminate)
{
  size_t index;
  for (index = 0; index < clause->count; ++index) {
    if (clause->vars[index] == var) {
      clause->eliminate[index] = eliminate;
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_is_empty(struct clause* clause)
{
  size_t index;
  for (index = 0; index < clause->count; ++index) {
    if (!clause->eliminate[index])
      return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void clause_set_init(struct clause_set* set)
{
  set->num_vars = 0;
  set->count = 0;
  set->capacity = 0;
  set->array = NULL;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void clause_set_free(struct clause_set* set)
{
  size_t index;
  for (index = 0; index < set->count; ++index) {
    clause_free(&set->array[index]);
  }
  free(set->array);
  clause_set_init(set);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_set_add(struct clause_set* set, size_t* out_index)
{
  if (set->count + 1 > set->capacity) {
    size_t new_capacity = set->capacity + clause_set_chunksize;
    struct clause* new_array = realloc(set->array, sizeof(*new_array) * new_capacity);
    if (new_array == NULL)
      return false;
    set->capacity = new_capacity;
    set->array = new_array;
  }
  clause_init(&set->array[set->count]);
  *out_index = set->count;
  set->count++;
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
int clause_set_parse(struct clause_set* set, FILE* fp)
{
  // xxx: implement clause_set_parse()
  return 0;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_set_solve(struct clause_set* set, bool** out_values)
{
  *out_values = NULL;
  // xxx: implement clause_set_solve().
  return false;
}
