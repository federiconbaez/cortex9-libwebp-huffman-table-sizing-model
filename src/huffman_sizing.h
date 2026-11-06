/* Size-only model of libwebp's BuildHuffmanTable (CVE-2023-4863).
 *
 * It counts HuffmanCode entries; it never allocates or writes a table, so it
 * cannot overflow anything. It reports how far the *pre-fix* code would
 * write into a fixed buffer, and that the fix rejects or sizes exactly.
 */
#ifndef CX9_HUFFMAN_SIZING_H
#define CX9_HUFFMAN_SIZING_H

#define CX9_MAX_ALLOWED_CODE_LENGTH 15
#define CX9_HUFFMAN_TABLE_BITS 8 /* main tables (huffman_utils.h:25) */
#define CX9_LENGTHS_TABLE_BITS 7 /* code-length table (huffman_utils.h:28) */

typedef enum {
  CX9_FULL = 0,          /* full prefix code: BuildHuffmanTable returns size */
  CX9_SINGLE = 1,        /* exactly one symbol: special case, returns root size */
  CX9_INCOMPLETE = 2,    /* Kraft sum < 1: final num_nodes check fails */
  CX9_OVERSUBSCRIBED = 3,/* num_open < 0 at some length */
  CX9_BAD_LENGTH = 4,    /* a length > 15 or count[len] > 1 << len */
  CX9_ALL_ZERO = 5
} cx9_tree;

typedef struct {
  cx9_tree tree;
  /* Entries the PRE-fix code writes from the table start before it returns.
   * Root replication fills the root table; 2nd-level tables are appended. The
   * pre code writes these BEFORE its final full-tree check, so an incomplete
   * code still writes them and only then returns 0. */
  int pre_extent;
  /* Total entries a full code needs (fix counting pass); 0 if not full. */
  int full_size;
  int second_tables;
  /* Length at which over-subscription was detected (0 if none). */
  int oversub_len;
} cx9_build;

/* Faithful port of BuildHuffmanTable's control flow on counts only
 * (huffman_utils.c at 902bc91 / its parent). code_lengths has n entries. */
cx9_build cx9_build_table(const int *code_lengths, int n, int root_bits);

#endif
