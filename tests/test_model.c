/* Model vs independent expectations (fixtures/expected.tsv, from the Python
 * port), invariants, and every PRACTICE.md answer (practice_level_N). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "model.h"

static int failures = 0, checks = 0;
#define CHECK(c, ...) do { ++checks; if (!(c)) { ++failures; \
  fprintf(stderr, "FAIL %s:%d: ", __FILE__, __LINE__); \
  fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)

static int parse(const char *s, int *c) {
  int i = 0; char buf[256], *t, *sv = NULL;
  strncpy(buf, s, sizeof buf - 1); buf[sizeof buf - 1] = 0;
  for (t = strtok_r(buf, ",", &sv); t && i < 15; t = strtok_r(NULL, ",", &sv)) c[i++] = atoi(t);
  return i;
}

static void test_fixtures(const char *dir) {
  char p[1024], a[1024], b[1024];
  FILE *fc, *fe;
  int cases = 0;
  snprintf(p, sizeof p, "%s/cases.tsv", dir); fc = fopen(p, "r");
  snprintf(p, sizeof p, "%s/expected.tsv", dir); fe = fopen(p, "r");
  CHECK(fc && fe, "open fixtures");
  if (!fc || !fe) return;
  if (!fgets(b, sizeof b, fe)) return; /* header */
  while (fgets(a, sizeof a, fc)) {
    char *n, *al, *r, *cs, *sv = NULL, *en, *et, *ee, *ef, *sv2 = NULL;
    int c[15]; cx9_outcome o;
    if (a[0] == '#' || a[0] == '\n') continue;
    n = strtok_r(a, "\t", &sv); al = strtok_r(NULL, "\t", &sv);
    r = strtok_r(NULL, "\t", &sv); cs = strtok_r(NULL, "\t\n", &sv);
    CHECK(fgets(b, sizeof b, fe) != NULL, "expected row for %s", n);
    en = strtok_r(b, "\t", &sv2); et = strtok_r(NULL, "\t", &sv2);
    ee = strtok_r(NULL, "\t", &sv2); ef = strtok_r(NULL, "\t\n", &sv2);
    CHECK(parse(cs, c) == 15, "15 counts in %s", n);
    o = cx9_run(CX9_PRE, c, atoi(al), atoi(r));
    CHECK(!strcmp(n, en), "row order %s vs %s", n, en);
    CHECK(!strcmp(cx9_tree_name(o.b.tree), et), "%s tree %s vs %s", n, cx9_tree_name(o.b.tree), et);
    CHECK(o.b.pre_extent == atoi(ee), "%s pre_extent %d vs %s", n, o.b.pre_extent, ee);
    CHECK(o.b.full_size == atoi(ef), "%s full_size %d vs %s", n, o.b.full_size, ef);
    ++cases;
  }
  CHECK(cases == 7, "7 fixture cases, got %d", cases);
  CHECK(fgets(b, sizeof b, fe) == NULL, "no extra expected rows");
  fclose(fc); fclose(fe);
}

/* Invariant: FIX never writes past what it allocates; for every FULL code
 * the precomputed bound holds (kTableSize is correct for full codes). */
static void test_invariants(void) {
  unsigned long long s = 99;
  long it;
  for (it = 0; it < 200000; ++it) {
    int c[15] = {0}, n = 0, l; long kr = 0, one = 1L << 15;
    for (l = 1; l <= 15 && n < 256; ++l) {
      long room = (one - kr) >> (15 - l);
      int mx = (int)(room > 256 - n ? 256 - n : room), k;
      s = s * 6364136223846793005ULL + 1; k = mx > 0 ? (int)((s >> 33) % (unsigned)(mx + 1)) : 0;
      c[l - 1] = k; n += k; kr += (long)k << (15 - l);
    }
    {
      cx9_outcome f = cx9_run(CX9_FIX, c, 256, 8), p = cx9_run(CX9_PRE, c, 256, 8);
      CHECK(f.label == CX9_EXACT_ALLOC || f.label == CX9_REJECTED_BEFORE, "fix label");
      if (f.label == CX9_REJECTED_BEFORE) CHECK(f.written == 0, "fix writes nothing when rejected");
      if (p.b.tree == CX9_FULL) CHECK(p.b.full_size <= 630, "full code within enough bound: %d", p.b.full_size);
    }
  }
}

/* Counterfactual control: the code-length table (3-bit lengths, <= 7, root 7)
 * never opens a 2nd-level table, so it never exceeds table[128], pre included. */
static void test_lengths_table_control(void) {
  unsigned long long s = 7; long it;
  for (it = 0; it < 100000; ++it) {
    int c[15] = {0}, n = 0, l; long kr = 0, one = 1L << 15;
    for (l = 1; l <= 7 && n < 19; ++l) {
      long room = (one - kr) >> (15 - l);
      int mx = (int)(room > 19 - n ? 19 - n : room), k;
      s = s * 6364136223846793005ULL + 1; k = mx > 0 ? (int)((s >> 33) % (unsigned)(mx + 1)) : 0;
      c[l - 1] = k; n += k; kr += (long)k << (15 - l);
    }
    { cx9_outcome o = cx9_run(CX9_PRE, c, 19, 7);
      CHECK(o.b.pre_extent <= 128 && o.past_budget == 0, "lengths table extent %d", o.b.pre_extent);
      CHECK(o.b.second_tables == 0, "no 2nd-level table for lengths <= 7"); }
  }
}

static const int W05[15] = {0,1,1,5,3,4,0,3,11,49,29,134,7,0,9};

static void practice(void) {
  int one[15] = {0}, full8[15] = {0};
  one[7] = 1; full8[7] = 256;
  /* L1: one symbol -> root replicated, 256 entries */
  { cx9_outcome o = cx9_run(CX9_PRE, one, 256, 8);
    CHECK(o.b.tree == CX9_SINGLE && o.b.pre_extent == 256, "practice_level_1"); }
  /* L2: all lengths 8 -> full, no 2nd-level table */
  { cx9_outcome o = cx9_run(CX9_PRE, full8, 256, 8);
    CHECK(o.b.tree == CX9_FULL && o.b.second_tables == 0 && o.b.full_size == 256, "practice_level_2"); }
  /* L3: the search case is incomplete (Kraft < 1) */
  { cx9_outcome o = cx9_run(CX9_PRE, W05, 256, 8);
    CHECK(o.b.tree == CX9_INCOMPLETE, "practice_level_3"); }
  /* L4: pre writes 730 entries before rejecting: 100 past the 630 bound */
  { cx9_outcome o = cx9_run(CX9_PRE, W05, 256, 8);
    CHECK(o.written == 730 && o.past_budget == 100 && o.label == CX9_PAST_BUDGET, "practice_level_4"); }
  /* L5: fix rejects the same code before writing anything */
  { cx9_outcome o = cx9_run(CX9_FIX, W05, 256, 8);
    CHECK(o.label == CX9_REJECTED_BEFORE && o.written == 0, "practice_level_5"); }
  /* L6: a full code within the bound is allocated exactly by the fix */
  { int deep[15] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,2};
    cx9_outcome o = cx9_run(CX9_FIX, deep, 256, 8);
    CHECK(o.label == CX9_EXACT_ALLOC && o.written == 384, "practice_level_6"); }
  /* L7: the code-length table stays inside table[128] (3-bit lengths) */
  { int l7[15] = {0}; l7[6] = 19; cx9_outcome o = cx9_run(CX9_PRE, l7, 19, 7);
    CHECK(o.b.pre_extent == 128 && o.past_budget == 0, "practice_level_7"); }
  /* L8: an over-long length is rejected, nothing is guessed */
  { int bad[15] = {3}; cx9_outcome o = cx9_run(CX9_PRE, bad, 256, 8);
    CHECK(o.b.tree == CX9_BAD_LENGTH, "practice_level_8"); }
}

int main(int argc, char **argv) {
  test_fixtures(argc > 1 ? argv[1] : "fixtures");
  test_invariants();
  test_lengths_table_control();
  practice();
  printf("%d checks, %d failures\n", checks, failures);
  return failures ? 1 : 0;
}
