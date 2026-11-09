/* CLI: huffsize case <alphabet> <root_bits> <c1,...,c15>
 *      huffsize results <fixtures/cases.tsv> <out_dir>
 *      huffsize search <alphabet> <root_bits> <max_len> <iterations>  */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "model.h"

static void print_case(FILE *f, const char *name, const int *c, int alphabet, int root) {
  int v;
  for (v = 0; v < 2; ++v) {
    cx9_outcome o = cx9_run((cx9_version)v, c, alphabet, root);
    fprintf(f, "%s,%s,%d,%d,%s,%d,%d,%d,%d,%s\n", name, v ? "fix" : "pre", alphabet,
            root, cx9_tree_name(o.b.tree), o.b.pre_extent, o.b.full_size, o.written,
            o.past_budget, cx9_label_name(o.label));
  }
}

/* Deterministic LCG so the search is reproducible on every platform. */
static unsigned long long lcg = 0x2545F4914F6CDD1DULL;
static unsigned next_rand(void) {
  lcg = lcg * 6364136223846793005ULL + 1442695040888963407ULL;
  return (unsigned)(lcg >> 33);
}

/* Random histograms with Kraft sum <= 1, lengths <= max_len. Reports the
 * largest PRE extent seen per tree kind. A search, not a proof of maximum. */
static void search(FILE *f, int alphabet, int root, int max_len, long iters) {
  int best[CX9_BAD_INPUT + 1] = {0}, bestc[CX9_BAD_INPUT + 1][15];
  long it;
  memset(bestc, 0, sizeof bestc);
  for (it = 0; it < iters; ++it) {
    int c[15] = {0}, n = 0, l;
    long kr = 0, one = 1L << 15;
    for (l = 1; l <= max_len && n < alphabet; ++l) {
      long room = (one - kr) >> (15 - l);
      int maxc = (int)(room > alphabet - n ? alphabet - n : room), k;
      if (maxc < 0) maxc = 0;
      k = maxc > 0 ? (int)(next_rand() % (unsigned)(maxc + 1)) : 0;
      if (l > root && next_rand() % 3 == 0) k = maxc;
      c[l - 1] = k;
      n += k;
      kr += (long)k << (15 - l);
    }
    {
      cx9_outcome o = cx9_run(CX9_PRE, c, alphabet, root);
      if (o.b.pre_extent > best[o.b.tree]) {
        best[o.b.tree] = o.b.pre_extent;
        memcpy(bestc[o.b.tree], c, sizeof c);
      }
    }
  }
  {
    int t, l;
    for (t = 0; t <= CX9_BAD_INPUT; ++t) {
      if (!best[t]) continue;
      fprintf(f, "%d,%d,%d,%ld,%s,%d,", alphabet, root, max_len, iters,
              cx9_tree_name((cx9_tree)t), best[t]);
      for (l = 0; l < 15; ++l) fprintf(f, "%d%s", bestc[t][l], l < 14 ? " " : "\n");
    }
  }
}

static int results(const char *cases, const char *out) {
  char path[4096], line[1024];
  FILE *in = fopen(cases, "r"), *f;
  if (!in) return 1;
  if (mkdir(out, 0755) != 0 && errno != EEXIST) return 1;
  snprintf(path, sizeof path, "%s/cases.csv", out);
  f = fopen(path, "w");
  if (!f) return 1;
  fprintf(f, "case,version,alphabet,root_bits,tree,pre_extent,full_size,written,past_budget,label\n");
  while (fgets(line, sizeof line, in)) {
    char *name, *alpha, *root, *counts, *extra, *save = NULL;
    int c[15], a, r;
    size_t ln = strlen(line);
    if (ln > 0 && line[ln - 1] != '\n' && !feof(in)) {
      fprintf(stderr, "line too long\n");
      fclose(in); fclose(f); return 1;
    }
    if (line[0] == '#' || line[0] == '\n') continue;
    name = strtok_r(line, "\t", &save);
    alpha = strtok_r(NULL, "\t", &save);
    root = strtok_r(NULL, "\t", &save);
    counts = strtok_r(NULL, "\t\n", &save);
    if (!name || !alpha || !root || !counts ||
        cx9_parse_int(alpha, 1, CX9_MAX_SYMBOLS, &a) ||
        cx9_parse_int(root, CX9_MIN_ROOT_BITS, CX9_MAX_ROOT_BITS, &r) ||
        cx9_parse_counts(counts, c)) {
      fprintf(stderr, "malformed case line: %s\n", name ? name : "(empty)");
      fclose(in); fclose(f); return 1;
    }
    extra = strtok_r(NULL, "\n", &save); /* the note column is free text */
    (void)extra;
    print_case(f, name, c, a, r);
  }
  fclose(in);
  fclose(f);
  snprintf(path, sizeof path, "%s/search.csv", out);
  f = fopen(path, "w");
  if (!f) return 1;
  fprintf(f, "alphabet,root_bits,max_len,iterations,tree,max_pre_extent,counts_len1_to_15\n");
  search(f, 256, 8, 15, 400000);
  search(f, 40, 8, 15, 400000);
  search(f, 19, 7, 7, 400000); /* code-length table: 3-bit lengths, <= 7 */
  fclose(f);
  return 0;
}

static int usage(void) {
  fprintf(stderr, "usage: huffsize case <alphabet> <root_bits> <c1,...,c15>\n"
                  "       huffsize results <cases.tsv> <out_dir>\n");
  return 2;
}

int main(int argc, char **argv) {
  if (argc == 5 && !strcmp(argv[1], "case")) {
    int c[15], a, r;
    if (cx9_parse_int(argv[2], 1, CX9_MAX_SYMBOLS, &a) ||
        cx9_parse_int(argv[3], CX9_MIN_ROOT_BITS, CX9_MAX_ROOT_BITS, &r) ||
        cx9_parse_counts(argv[4], c))
      return usage();
    printf("case,version,alphabet,root_bits,tree,pre_extent,full_size,written,past_budget,label\n");
    print_case(stdout, "cli", c, a, r);
    return 0;
  }
  if (argc == 4 && !strcmp(argv[1], "results")) return results(argv[2], argv[3]);
  return usage();
}
