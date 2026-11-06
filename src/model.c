#include "model.h"

int cx9_enough_bound(int alphabet, int root_bits) {
  if (root_bits == 8 && alphabet == 256) return 630;
  if (root_bits == 8 && alphabet == 40) return 410;
  if (root_bits == 8 && alphabet == 280) return 654;
  if (root_bits == 7 && alphabet == 19) return 1 << 7; /* table[128] */
  return -1;
}

const char *cx9_tree_name(cx9_tree t) {
  static const char *n[] = {"FULL", "SINGLE", "INCOMPLETE", "OVERSUBSCRIBED",
                            "BAD_LENGTH", "ALL_ZERO"};
  return n[t];
}

const char *cx9_label_name(cx9_label l) {
  static const char *n[] = {"WITHIN", "PAST_BUDGET", "REJECTED_BEFORE_WRITE",
                            "EXACT_ALLOC", "NO_BOUND"};
  return n[l];
}

/* Expand a count histogram into a code_lengths array (symbol order does not
 * change any size). counts15[i] is the number of codes of length i + 1. */
static int expand(const int *counts15, int alphabet, int *cl) {
  int idx = 0, len, k;
  for (len = 1; len <= CX9_MAX_ALLOWED_CODE_LENGTH; ++len)
    for (k = 0; k < counts15[len - 1]; ++k) {
      if (idx >= alphabet) return -1;
      cl[idx++] = len;
    }
  while (idx < alphabet) cl[idx++] = 0;
  return 0;
}

cx9_outcome cx9_run(cx9_version v, const int *counts15, int alphabet, int root_bits) {
  cx9_outcome o;
  int cl[4096];
  o.bound = cx9_enough_bound(alphabet, root_bits);
  o.written = 0;
  o.past_budget = 0;
  o.label = CX9_NO_BOUND;
  if (alphabet < 1 || alphabet > 4096 || expand(counts15, alphabet, cl) != 0) {
    o.b.tree = CX9_BAD_LENGTH;
    o.b.pre_extent = o.b.full_size = o.b.second_tables = o.b.oversub_len = 0;
    return o;
  }
  o.b = cx9_build_table(cl, alphabet, root_bits);
  if (v == CX9_PRE) {
    /* pre writes pre_extent entries, then may still return 0 */
    o.written = o.b.pre_extent;
    if (o.bound < 0) return o;
    o.past_budget = o.written > o.bound ? o.written - o.bound : 0;
    o.label = o.past_budget > 0 ? CX9_PAST_BUDGET : CX9_WITHIN;
  } else {
    /* fix: NULL pass first (huffman_utils.c:219-221). 0 -> no write. */
    if (o.b.tree != CX9_FULL && o.b.tree != CX9_SINGLE) {
      o.label = CX9_REJECTED_BEFORE;
      return o;
    }
    /* a new segment of max(total, segment) when it does not fit (224-251) */
    o.written = o.b.full_size;
    o.label = CX9_EXACT_ALLOC;
  }
  return o;
}
