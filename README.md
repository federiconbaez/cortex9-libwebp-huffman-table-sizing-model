# cortex9-libwebp-huffman-table-sizing-model

Size-only model of **CVE-2023-4863**, the libwebp lossless Huffman-table
heap overflow (chromium:1479274, fixed by 902bc91 "Fix OOB write in
BuildHuffmanTable"). It is written in C, educational, offline and
deterministic. It **counts table entries**. It never allocates or writes a
Huffman table, reads no image, builds no WebP file and links no libwebp.

*Versión en español: [README.es.md](README.es.md).*

## The bug, from the source

- **Pre-fix:** the decoder pre-allocates `num_htree_groups * kTableSize[...]`
  entries (`vp8l_dec.c`, parent 7ba44f8). `kTableSize` sums per-alphabet
  maxima "computed ... with Mark Adler's tool" `enough.c`: 630 for 256
  symbols, 410 for 40, 654 for the 280-symbol green alphabet. Those maxima
  are for **full** prefix codes.
- **Where it goes wrong:** pre-fix `BuildHuffmanTable` (`huffman_utils.c`)
  opens and writes the 2nd-level tables **as codes arrive** (lines
  165-191). It checks whether the tree is full only at the end (lines
  194-196). An **incomplete** code (Kraft sum < 1) makes `NextTableBitSize`
  run to length 15, so the tables grow past the full-code bound. They are
  written, and only then does the function return 0.
- **Fix:** a counting pass with `root_table == NULL` runs first and returns
  0 for a non-full code **before any write**. It also returns the true size,
  and the decoder allocates a new segment when that size does not fit
  (lines 219-251).

**Correction to the selection hypothesis.** "Huffman sizing" is right, but
the exact cause is *writing before the full-tree check*. The code-length
table `table[1 << 7]` (`vp8l_dec.c:265`) is **not** the overflowing buffer.
Its lengths are read with 3 bits (`vp8l_dec.c:346`, so ≤ 7 = root bits) and
it never opens a 2nd-level table. The model checks this as a control.

## What the model does

- `src/huffman_sizing.c` is a port of `BuildHuffmanTable` on counts only:
  `NextTableBitSize`, `GetNextKey`, the sort loop's `offset` updates, the
  num_open and num_nodes checks. It returns
  - `pre_extent`: the entries the pre-fix code writes before returning;
  - `full_size`: what the fix counts for a full code.
- `src/model.c` labels each case against the `enough.c` bound:
  - pre-fix: `WITHIN` or `PAST_BUDGET`, an abstract count;
  - fix: `REJECTED_BEFORE_WRITE` or `EXACT_ALLOC`.
- `scripts/derive_expected.py` is a **second, separate port** in Python.
  It confirms full / incomplete with exact-fraction Kraft sums. The two
  ports disagreed once during development: the C port lacked the sort
  loop's `offset[len]++`. The C port was fixed to match the source.

## Results (`results/`, regenerated in CI)

| case | tree | pre-fix writes | bound | fix |
|---|---|---|---|---|
| `w05` (found by search) | INCOMPLETE | **730** | 630 | rejected before write |
| full chain to length 15 | FULL | 384 | 630 | exact allocation |
| code-length table, lengths 7 | INCOMPLETE | 128 | 128 | rejected before write |

The search (`results/search.csv`, 400,000 seeded histograms per row) found
these maxima, but it is a search, not a proof:

| alphabet | largest full code found | largest incomplete code found | bound |
|---|---|---|---|
| 256 | 592 | 726 | 630 |
| 40 (distance) | 402 | **512** | 410 |
| 19-symbol code-length table | 128 | 128 | 128 |

- Full codes found always stay within the bound.
- Incomplete codes exceed it in the pre-fix code.
- The code-length table never exceeds 128.

`tests/test_model.c` checks, under ASan/UBSan:

- that the fix never writes when it rejects;
- that 200,000 random full codes stay within 630;
- the code-length-table control (100,000 cases);
- every PRACTICE answer.

## Usage

```sh
sh scripts/build.sh test               # -Wall -Wextra -Werror + ASan/UBSan
sh scripts/build.sh results /tmp/r
./build/huffsize case 256 8 0,1,1,5,3,4,0,3,11,49,29,134,7,0,9
```

See [LIMITATIONS.md](LIMITATIONS.md), [SOURCES.md](SOURCES.md) and
[PRACTICE.md](PRACTICE.md) (Spanish). License: Apache-2.0.
