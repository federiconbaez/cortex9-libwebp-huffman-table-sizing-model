#ifndef CX9_MODEL_H
#define CX9_MODEL_H
#include "huffman_sizing.h"

/* Per-alphabet bounds kTableSize sums (vp8l_dec.c:80-102, "computed ... with
 * Mark Adler's tool" enough.c, for FULL codes): 630 for 256 symbols, 410 for
 * 40, 654 for the 280-symbol green alphabet at color_cache_bits 0. */
int cx9_enough_bound(int alphabet, int root_bits);

typedef enum { CX9_PRE = 0, CX9_FIX = 1 } cx9_version;

typedef enum {
  CX9_WITHIN = 0,          /* writes stay inside the budget */
  CX9_PAST_BUDGET = 1,     /* PRE: abstract write past the precomputed bound */
  CX9_REJECTED_BEFORE = 2, /* FIX: not full -> rejected before any write */
  CX9_EXACT_ALLOC = 3,     /* FIX: full code -> segment sized to total_size */
  CX9_NO_BOUND = 4         /* no precomputed bound for this alphabet/root */
} cx9_label;

typedef struct {
  cx9_build b;
  int bound;
  int written;     /* entries written (abstract) */
  int past_budget; /* entries past the precomputed bound (abstract) */
  cx9_label label;
} cx9_outcome;

cx9_outcome cx9_run(cx9_version v, const int *counts15, int alphabet, int root_bits);
const char *cx9_tree_name(cx9_tree t);
const char *cx9_label_name(cx9_label l);
#endif
