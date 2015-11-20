/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * dpll/dpll.c
 */

#define _FILE_OFFSET_BITS 64

#include "dpll.h"
#include <assert.h>
#include <ctype.h>  // isspace
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stdlib.h>

// MSVC does not support the %zu format specifier.
#ifdef _MSC_VER
# define PRsize_t "%lu"
#else
# define PRsize_t "%zu"
#endif

// MSVC does not supprt ftello(), fseeko() and off_t.
#ifdef _MSC_VER
# define ftello _ftelli64
# define fseeko _fseeki64
typedef __int64 off_t;
#endif

// The increment for the capacity of a clause and clause_set.
static size_t const clause_chunksize = 16;
static size_t const clause_set_chunksize = 128;

// Read a \c long \c long from a file and return true on success,
// false on error. Requires the file to be positioned immediately
// at the ASCII integer. Set #errno when false is returned.
//
// #ERANGE: If the ASCII integer in the file is too large.
// #EILSEQ: If the file does not point to a digit or a minus
//   followed by a digit.
static bool read_long_long(FILE* fp, long long* result);

// Skip non-newline whitespace in the file.
static void skip_whitespace(FILE* fp);

// Skip a specific character if it is the character currently pointed
// to in the file. Return true if the character was skipped, false if not.
static bool skip_char(FILE* fp, int c);

// Match the specified text from the current position in the file.
static bool match_token(FILE* fp, const char* str);

// Internal procedure to solve a clause_set.
static bool _clause_set_solve(
  struct clause_set* set, bool* out_values, bool* pre_cond, long long last_var,
  clause_set_solve_callback callback, void* userdata, bool* sat, bool* search);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
char dpll_errinfo[BUFSIZ] = {0};

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void dpll_puterr(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  vsnprintf(dpll_errinfo, BUFSIZ, fmt, args);
  va_end(args);
}

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
  // Check if the literal already exist.
  size_t index;
  for (index = 0; index < clause->count; ++index) {
    if (clause->vars[index] == var)
      return 1;
  }

  // Resize the data if necessary.
  if (clause->count + 1 > clause->capacity) {
    size_t new_capacity = clause->capacity + clause_chunksize;
    long long* new_vars = realloc(clause->vars, sizeof(*new_vars) * new_capacity);
    int* new_eliminate = realloc(clause->eliminate, sizeof(*new_eliminate) * new_capacity);
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
  clause->eliminate[clause->count] = 0;
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
    int* new_eliminate = realloc(clause->eliminate, sizeof(*new_eliminate) * clause->count);
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
bool clause_is_empty(struct clause* clause)
{
  size_t index;
  for (index = 0; index < clause->count; ++index) {
    if (clause->eliminate[index] == 0)
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
  set->eliminate = NULL;
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
  free(set->eliminate);
  clause_set_init(set);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_set_add(struct clause_set* set, size_t* out_index)
{
  if (set->count + 1 > set->capacity) {
    size_t new_capacity = set->capacity + clause_set_chunksize;
    struct clause* new_array = realloc(set->array, sizeof(*new_array) * new_capacity);
    int* new_eliminate = realloc(set->eliminate, sizeof(*new_eliminate) * new_capacity);
    if (new_array == NULL || new_eliminate == NULL) {
      free(new_array);
      free(new_eliminate);
      clause_set_free(set);
      return false;
    }
    set->capacity = new_capacity;
    set->array = new_array;
    set->eliminate = new_eliminate;
  }
  clause_init(&set->array[set->count]);
  *out_index = set->count;
  set->count++;
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_set_parse(struct clause_set* set, FILE* fp)
{
  int res;
  long long value, num_vars, num_clauses;
  static size_t size_max = (size_t) -1;

  // Skip all comment lines.
  while (skip_char(fp, 'c')) {
    int c;
    while ((c = getc(fp)) != '\n')
      ;
  }

  // Read the pre conditions.
  if (!skip_char(fp, 'p')) {
    dpll_puterr("clause_set_parse(): expected 'p' instruction");
    return false;
  }
  skip_whitespace(fp);
  if (!match_token(fp, "cnf")) {
    dpll_puterr("clause_set_parse(): expected 'cnf' token");
    return false;
  }
  skip_whitespace(fp);
  if (!read_long_long(fp, &num_vars)) {
    dpll_puterr("clause_set_parse(): expected num vars");
    return false;
  }
  skip_whitespace(fp);
  if (!read_long_long(fp, &num_clauses))
    num_clauses = 0;

  if (num_vars < 0 || num_vars > size_max) {
    errno = EINVAL;
    dpll_puterr("clause_set_parse(): literal count too big or not positive");
    return false;
  }
  set->num_vars = (size_t) num_vars;

  skip_whitespace(fp);
  if (!skip_char(fp, '\n')) {
    // expected newline after number of literals.
    errno = EINVAL;
    dpll_puterr("clause_set_parse(): expected newline after 'p' instruction, got %c", getc(fp));
    return false;
  }

  // Read a clause from each line. Since empty clauses don't make
  // much sense in a CNF (as they evaluate to false), we simply
  // skip empty lines.
  while (!feof(fp)) {
    size_t index = 0;
    struct clause* clause = NULL;

    skip_whitespace(fp);
    if (skip_char(fp, '\n') || skip_char(fp, EOF))
      continue;

    if (!clause_set_add(set, &index)) {
      errno = ENOMEM;
      dpll_puterr("clause_set_parse(): could not add new clause");
      return false;
    }
    clause = &set->array[index];

    while (true) {
      skip_whitespace(fp);
      if (skip_char(fp, '0')) {
        skip_whitespace(fp);
        if (!skip_char(fp, '\n')) {
          dpll_puterr("clause_set_parse(): clause #"PRsize_t": expected newline after clause terminator", index);
          return false;
        }
        break;
      }

      if (!read_long_long(fp, &value)) {
        errno = EINVAL;
        dpll_puterr("clause_set_parse(): clause #"PRsize_t": expected literal ", index);
        return false;
      }

      if (value < -num_vars || value > num_vars || value == 0) {
        errno = EINVAL;
        dpll_puterr(
          "clause_set_parse(): clause #"PRsize_t": literal "
          "'%llu' not in range", index, value);
        return false;
      }

      res = clause_add(clause, value);
      if (res == -1) {
        errno = ENOMEM;
        dpll_puterr("clause_set_parse(): could not add literal to clause");
        return false;
      }
      else if (res == 1) {
        dpll_puterr(
          "clause_set_parse(): warning: clause #"PRsize_t": "
          "contains duplicate literal '%llu'", index, value);
      }
      else assert(res == 0);
      clause_shrink_to_fit(clause);
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void clause_set_format(struct clause_set* set, FILE* fp)
{
  size_t index;
  fprintf(fp, PRsize_t"\n", set->num_vars);
  for (index = 0; index < set->count; ++index) {
    size_t j;
    struct clause* clause = &set->array[index];
    for (j = 0; j < clause->count; ++j) {
      printf("%lld ", clause->vars[j]);
    }
    printf("\n");
  }
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_set_is_empty(struct clause_set* set)
{
  size_t index;
  for (index = 0; index < set->count; ++index) {
    if (set->eliminate[index] == 0)
      return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_set_eliminate(struct clause_set* set, long long var, bool reset)
{
  int add = (reset ? -1 : 1);
  size_t index;
  for (index = 0; index < set->count; ++index) {
    size_t j;
    struct clause* clause = &set->array[index];
    for (j = 0; j < clause->count; ++j) {
      if (clause->vars[j] == var) {
        set->eliminate[index] += add;
      }
      else if (clause->vars[j] == -var) {
        clause->eliminate[j] += add;
        if (!reset && clause_is_empty(clause))
          return false;
      }
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool clause_set_solve(
  struct clause_set* set, bool** out_values,
  clause_set_solve_callback callback, void* userdata)
{
  bool sat = false;
  bool search = true;
  bool* pre_cond = malloc(sizeof(bool) * set->num_vars);
  *out_values = malloc(sizeof(bool) * set->num_vars);
  if (!*out_values || !pre_cond) {
    return false;
  }
  _clause_set_solve(set, *out_values, pre_cond, 0, callback, userdata, &sat, &search);
  if (!sat) {
    free(*out_values);
    *out_values = NULL;
  }
  return sat;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool _clause_set_solve(
  struct clause_set* set, bool* out_values, bool* pre_cond, long long last_var,
  clause_set_solve_callback callback, void* userdata, bool* sat, bool* search)
{
  long long curr_var = last_var + 1;
  assert(last_var <= set->num_vars);

  if (!*search)
    return false;

  if (last_var == set->num_vars) {
    if (clause_set_is_empty(set)) {
      *sat = true;
      if (callback)
        *search = callback(set->num_vars, out_values, userdata);
      else
        *search = false;
      return true;
    }
    return false;
  }

  // xxx: eventually find a better literal than just the next. (ie. from
  // a clause that only contains that literal). Use pre_cond for that!

  out_values[curr_var - 1] = true;
  if (clause_set_eliminate(set, curr_var, false) && _clause_set_solve(set, out_values, pre_cond, curr_var, callback, userdata, sat, search)) {
    if (!*search)
      return true;
  }
  clause_set_eliminate(set, curr_var, true);  // revert

  out_values[curr_var - 1] = false;
  if (clause_set_eliminate(set, -curr_var, false) && _clause_set_solve(set, out_values, pre_cond, curr_var, callback, userdata, sat, search)) {
    if (!*search)
      return true;
  }
  clause_set_eliminate(set, -curr_var, true);  // revert
  return false;;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
static bool read_long_long(FILE* fp, long long* result)
{
  int c;
  size_t const max_digits = floor(log10(LLONG_MAX)) + 1;
  size_t digits = 0;
  bool negative = false;

  *result = 0;
  c = getc(fp);
  if (c == '-') {
    negative = true;
    c = getc(fp);
  }

  while (c >= '0' && c <= '9') {
    if (digits == max_digits) {
      errno = ERANGE;
      return false;
    }
    digits++;
    *result = *result * 10 + c - '0';
    c = getc(fp);
  }

  ungetc(c, fp);
  if (negative)
    *result *= -1;
  if (digits == 0) {
    errno = EILSEQ;
    return false;
  }
  return true;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void skip_whitespace(FILE* fp)
{
  int c;
  while (isspace(c = getc(fp)) && c != '\n')
    ;
  ungetc(c, fp);
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool skip_char(FILE* fp, int c)
{
  int h = getc(fp);
  if (h == c)
    return true;
  ungetc(h, fp);
  return false;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
bool match_token(FILE* fp, const char* str)
{
  off_t pos = ftello(fp);
  int c;
  while (*str && (c = getc(fp)) == *str) {
    str++;
  }
  if (!*str) return true;
  fseeko(fp, pos, 0);
  return false;
}
