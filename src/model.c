#include <errno.h>
#include <stdlib.h>
#include <string.h>
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
                            "BAD_LENGTH", "ALL_ZERO", "BAD_INPUT"};
  if ((int)t < 0 || (int)t >= (int)(sizeof n / sizeof n[0])) return "UNKNOWN";
  return n[t];
}

const char *cx9_label_name(cx9_label l) {
  static const char *n[] = {"WITHIN", "PAST_BUDGET", "REJECTED_BEFORE_WRITE",
                            "EXACT_ALLOC", "NO_BOUND"};
  if ((int)l < 0 || (int)l >= (int)(sizeof n / sizeof n[0])) return "UNKNOWN";
  return n[l];
}

/* Expand a count histogram into a code_lengths array (symbol order does not
 * change any size). counts15[i] is the number of codes of length i + 1. */
static int expand(const int *counts15, int alphabet, int *cl) {
  int idx = 0, len, k;
  long sum = 0;
  /* validate the whole histogram before touching cl: each count 0..alphabet,
   * sum <= alphabet (a long cannot overflow on 15 values <= 4096). */
  for (len = 0; len < CX9_MAX_ALLOWED_CODE_LENGTH; ++len) {
    if (counts15[len] < 0 || counts15[len] > alphabet) return -1;
    sum += counts15[len];
  }
  if (sum > alphabet) return -1;
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
  int cl[CX9_MAX_SYMBOLS];
  o.bound = cx9_enough_bound(alphabet, root_bits);
  o.written = 0;
  o.past_budget = 0;
  o.label = CX9_NO_BOUND;
  if (!counts15 || alphabet < 1 || alphabet > CX9_MAX_SYMBOLS ||
      root_bits < CX9_MIN_ROOT_BITS || root_bits > CX9_MAX_ROOT_BITS) {
    o.b.tree = CX9_BAD_INPUT;
    o.b.pre_extent = o.b.full_size = o.b.second_tables = o.b.oversub_len = 0;
    return o;
  }
  if (expand(counts15, alphabet, cl) != 0) {
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

/* Strict decimal int in [lo, hi]; the whole token must be digits. */
int cx9_parse_int(const char *s, int lo, int hi, int *out) {
  char *end;
  long v;
  if (!s || !*s) return -1;
  /* strtol skips leading whitespace and accepts '+': refuse both */
  if (!(*s == '-' || (*s >= '0' && *s <= '9'))) return -1;
  errno = 0;
  v = strtol(s, &end, 10);
  if (errno || *end || v < lo || v > hi) return -1;
  *out = (int)v;
  return 0;
}

/* Exactly 15 comma-separated counts, each 0..CX9_MAX_SYMBOLS. */
int cx9_parse_counts(const char *s, int *c) {
  int i = 0;
  char buf[256], *tok, *save = NULL;
  size_t n;
  if (!s) return -1;
  n = strlen(s);
  if (n == 0 || n >= sizeof buf || s[n - 1] == ',' || s[0] == ',' ||
      strstr(s, ",,"))
    return -1;
  memcpy(buf, s, n + 1);
  for (tok = strtok_r(buf, ",", &save); tok; tok = strtok_r(NULL, ",", &save)) {
    if (i >= 15 || cx9_parse_int(tok, 0, CX9_MAX_SYMBOLS, &c[i])) return -1;
    i++;
  }
  return i == 15 ? 0 : -1;
}
