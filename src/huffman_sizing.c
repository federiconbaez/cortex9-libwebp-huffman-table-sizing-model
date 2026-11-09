#include "huffman_sizing.h"

/* NextTableBitSize (huffman_utils.c:66-76). For an incomplete code `left`
 * never reaches 0, so len runs to 15 and the table is 1 << (15 - root). */
static int next_table_bit_size(const int *count, int len, int root_bits) {
  int left = 1 << (len - root_bits);
  while (len < CX9_MAX_ALLOWED_CODE_LENGTH) {
    left -= count[len];
    if (left <= 0) break;
    ++len;
    left <<= 1;
  }
  return len - root_bits;
}

/* GetNextKey (huffman_utils.c:46-54). */
static int get_next_key(int key, int len) {
  int step = 1 << (len - 1);
  while (key & step) step >>= 1;
  return step ? (key & (step - 1)) + step : key;
}

cx9_build cx9_build_table(const int *code_lengths, int n, int root_bits) {
  cx9_build r;
  int total;
  int count[CX9_MAX_ALLOWED_CODE_LENGTH + 1] = {0};
  int offset[CX9_MAX_ALLOWED_CODE_LENGTH + 1];
  int len, s;

  r.tree = CX9_BAD_LENGTH;
  r.pre_extent = 0;
  r.full_size = 0;
  r.second_tables = 0;
  r.oversub_len = 0;

  if (!code_lengths || n < 1 || n > CX9_MAX_SYMBOLS ||
      root_bits < CX9_MIN_ROOT_BITS || root_bits > CX9_MAX_ROOT_BITS) {
    r.tree = CX9_BAD_INPUT;
    return r;
  }
  total = 1 << root_bits;
  /* histogram (lines 100-105): no write yet */
  for (s = 0; s < n; ++s) {
    if (code_lengths[s] < 0 || code_lengths[s] > CX9_MAX_ALLOWED_CODE_LENGTH)
      return r;
    ++count[code_lengths[s]];
  }
  if (count[0] == n) {
    r.tree = CX9_ALL_ZERO;
    return r;
  }
  offset[1] = 0;
  for (len = 1; len < CX9_MAX_ALLOWED_CODE_LENGTH; ++len) {
    if (count[len] > (1 << len)) return r; /* lines 114-116 */
    offset[len + 1] = offset[len] + count[len];
  }
  /* Sort loop (lines 120-129) with sorted == NULL: offset[len]++ per symbol.
   * Afterwards offset[15] is the number of non-zero-length symbols, which the
   * single-symbol test and the full-tree test below rely on. */
  for (s = 0; s < n; ++s)
    if (code_lengths[s] > 0) offset[code_lengths[s]]++;
  if (offset[CX9_MAX_ALLOWED_CODE_LENGTH] == 1) {
    /* lines 131-139: replicate the one value over the root, return root size */
    r.tree = CX9_SINGLE;
    r.pre_extent = total;
    r.full_size = total;
    return r;
  }
  {
    int key = 0, low = -1, num_nodes = 1, num_open = 1;
    int mask = total - 1;
    /* root table: ReplicateValue fills all 1 << root_bits entries */
    for (len = 1; len <= root_bits; ++len) {
      num_open <<= 1;
      num_nodes += num_open;
      num_open -= count[len];
      if (num_open < 0) {
        r.tree = CX9_OVERSUBSCRIBED;
        r.oversub_len = len;
        r.pre_extent = total; /* root region only */
        return r;
      }
      for (; count[len] > 0; --count[len]) key = get_next_key(key, len);
    }
    /* 2nd-level tables (lines 165-191): opened and written as codes arrive */
    for (len = root_bits + 1; len <= CX9_MAX_ALLOWED_CODE_LENGTH; ++len) {
      num_open <<= 1;
      num_nodes += num_open;
      num_open -= count[len];
      if (num_open < 0) {
        r.tree = CX9_OVERSUBSCRIBED;
        r.oversub_len = len;
        r.pre_extent = total;
        return r;
      }
      for (; count[len] > 0; --count[len]) {
        if ((key & mask) != low) {
          /* at most CX9_MAX_SYMBOLS tables of at most 1 << 14 entries:
           * total stays below 2^27, no int overflow */
          total += 1 << next_table_bit_size(count, len, root_bits);
          ++r.second_tables;
          low = key & mask;
        }
        key = get_next_key(key, len);
      }
    }
    r.pre_extent = total; /* written before the check below */
    /* "Check if tree is full" (lines 194-196) */
    if (num_nodes != 2 * offset[CX9_MAX_ALLOWED_CODE_LENGTH] - 1) {
      r.tree = CX9_INCOMPLETE;
      return r;
    }
    r.tree = CX9_FULL;
    r.full_size = total;
  }
  return r;
}
