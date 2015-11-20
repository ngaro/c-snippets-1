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
  // The number of variables in #vars;
  size_t count;

  // The capacity of #vars. While the clause is built, this is likely
  // to be bigger than #count. After the clause has been built, the
  // memory can be shrinked with clause_shrink_to_fit().
  size_t capacity;

  // An array of #count variable indices. A negative value indicates that
  // the variable is negated.
  long long* vars;

  // An array of #count boolean values that indicate wether the respective
  // variable is eliminate from the clause.
  bool* eliminate;
};

// Initialize an empty clause.
void clause_init(struct clause* clause);

// Free a clause.
void clause_free(struct clause* clause);

// Append the specified variable to the clause. Return 1 if the variable
// is already in the clause, -1 on a memory error and 0 on success. Free
// the clause in case of a memory error.
int clause_add(struct clause* clause, long long const var);

// Shrink the buffers for clause#vars and clause#eliminate to fit the
// filled count. Return true on success, false on a memory error. Free
// the clause in case of a memory error.
bool clause_shrink_to_fit(struct clause* clause);

// Return true if all variables of the clause are eliminated, which
// is equal to an empty clause. An empty clause evaluates to false.
bool clause_is_empty(struct clause* clause);

//-----------------------------------------------------------------------------
// Represents a conjunctive normal form and a set of clauses.
//-----------------------------------------------------------------------------
struct clause_set
{
  // The number of variables used in the clauses. Must be updated manually
  // when new variables are introduced. Remember that the index of a variable
  // is offset by one, since we can't use the variable 0 as it can not be
  // negative.
  size_t num_vars;

  // The number of clauses present in the set.
  size_t count;

  // The capacity of the buffer.
  size_t capacity;

  // An array of #count clasues.
  struct clause* array;

  // An array of #count boolean values that indicate that wether the
  // respective clause is eliminated.
  bool* eliminate;
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

// Eliminate all clauses that contain the specified variable and
// eliminate all variables in all clauses that contain the invert
// of that variable. If \p reset is true, all changes made will
// be reverted.
// Return false if a clause is empty after this call.
bool clause_set_eliminate(struct clause_set* set, long long var, bool reset);

// Parse clauses from a FILE into a clause_set. The first line in
// the file must contain a single number, which is the number of
// variables used. All folllowing lines represent a clause each.
// Variable indices start at one (!!). Negated variables can be
// represented by the variable index * -1.
// Return true on success, false on error. #errno is set if the
// function returned false.
bool clause_set_parse(struct clause_set* set, FILE* fp);

// Format the clause, parsable by clause_set_parse().
void clause_set_format(struct clause_set* set, FILE* fp);

// Solve the specified clause_set to find the first possible solution.
// Return true if the clause_set is satisfiable, false if not. If true
// is returned, \p out_values is set and must be freed using free().
// Set #errno if an error occurs.
bool clause_set_solve(struct clause_set* set, bool** out_values);


#endif // DPLL_H_
