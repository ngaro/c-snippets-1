/* Copyright (C) 2015  Niklas Rosenstein
 * All rights reserved.
 *
 * dpll/dpll.h
 */

#ifndef DPLL_H_
#define DPLL_H_

#include <stdio.h>

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Latest error infromation. Allocated with malloc(). Modify with dpll_puterr().
extern char dpll_errinfo[BUFSIZ];

// Update the #dpll_errinfo, freeing the previous value.
void dpll_puterr(const char* fmt, ...);

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
enum bool
{
  false,
  true,
};
typedef enum bool bool;

//-----------------------------------------------------------------------------
// This struct represents a disjunctive clause of propositional logic.
//-----------------------------------------------------------------------------
struct clause
{
  // The number of literals in #vars;
  size_t count;

  // The capacity of #vars. While the clause is built, this is likely
  // to be bigger than #count. After the clause has been built, the
  // memory can be shrinked with clause_shrink_to_fit().
  size_t capacity;

  // An array of #count literal indices. A negative value indicates that
  // the literal is negated.
  long long* vars;

  // An array of #count values that indicate wether the respective
  // literal is eliminate from the clause. A non-zero value indicates
  // that the literal is eliminated.
  int* eliminate;
};

// Initialize an empty clause.
void clause_init(struct clause* clause);

// Free a clause.
void clause_free(struct clause* clause);

// Append the specified literal to the clause. Return 1 if the literal
// is already in the clause, -1 on a memory error and 0 on success. Free
// the clause in case of a memory error.
int clause_add(struct clause* clause, long long const var);

// Shrink the buffers for clause#vars and clause#eliminate to fit the
// filled count. Return true on success, false on a memory error. Free
// the clause in case of a memory error.
bool clause_shrink_to_fit(struct clause* clause);

// Return true if all literals of the clause are eliminated, which
// is equal to an empty clause. An empty clause evaluates to false.
bool clause_is_empty(struct clause* clause);

//-----------------------------------------------------------------------------
// Represents a conjunctive normal form and a set of clauses.
//-----------------------------------------------------------------------------
struct clause_set
{
  // The number of literals used in the clauses. Must be updated manually
  // when new literals are introduced. Remember that the index of a literal
  // is offset by one, since we can't use the literal 0 as it can not be
  // negative.
  size_t num_vars;

  // The number of clauses present in the set.
  size_t count;

  // The capacity of the buffer.
  size_t capacity;

  // An array of #count clasues.
  struct clause* array;

  // An array of #count values that indicate that wether the
  // respective clause is eliminated. A non-zero value indicates
  // that the clause is eliminated.
  int* eliminate;
};

// Initialize an empty clause_set.
void clause_set_init(struct clause_set* set);

// Free a clause set.
void clause_set_free(struct clause_set* set);

// Allocate a clause in the clause_set and return the clause index.
// Return true on succes, false on memory error.
bool clause_set_add(struct clause_set* set, size_t* out_index);

// Return true if the clause_set is empty.
bool clause_set_is_empty(struct clause_set* set);

// Eliminate all clauses that contain the specified literal and
// eliminate all literals in all clauses that contain the invert
// of that literal. If \p reset is true, all changes made will
// be reverted.
// Return false if a clause is empty after this call.
bool clause_set_eliminate(struct clause_set* set, long long var, bool reset);

// Parse a DIMACS CNF file from the specified FILE and fills the
// clause_set \p set. As an additional, this parser function does
// not require the clause count to be specified in the \c p
// line of the input file.
//
// For more information on the DIMACS CNF format, see:
//   http://logic.pdmi.ras.ru/~basolver/dimacs.html
//
// Return true on success, false on error. #errno is set if the
// function returned false.
bool clause_set_parse(struct clause_set* set, FILE* fp);

// Format the clause, parsable by clause_set_parse().
void clause_set_format(struct clause_set* set, FILE* fp);

typedef bool (*clause_set_solve_callback)(
  size_t num_vars, bool* out_values, void* userdata);

// Solve the specified clause_set to find the first possible solution.
// Return true if the clause_set is satisfiable, false if not. If true
// is returned, \p out_values is set and must be freed using free().
// Set #errno if an error occurs.
//
// If \p callback is specified, the function attempts to find all
// possible solutions until the callback returns false. The returned
// \p out_values will be the last solution found instead of the first.
bool clause_set_solve(
  struct clause_set* set, bool** out_values,
  clause_set_solve_callback callback, void* userdata);

#endif // DPLL_H_
