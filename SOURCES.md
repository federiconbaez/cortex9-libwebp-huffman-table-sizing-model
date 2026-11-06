# Sources

Local copies under `security-research-45/sources/next3/webp/` (not in this
repo), fetched read-only from GitHub (`webmproject/libwebp`) on 2026-11-06.
Parent of 902bc91: `7ba44f80f3b94fc0138db159afea770ef06532a0`.

| Source | Used for | SHA-256 of copy |
|---|---|---|
| libwebp commit 902bc9190331343b2017211debcec8d2ab87e17a patch ("Fix OOB write in BuildHuffmanTable.", 2023-09-07, Bug chromium:1479274) | the causal diff | bde34a71c4c378f92afe4e39082d553a4391776524a3ddcfcccfe7e39acb715d |
| `src/utils/huffman_utils.c` at parent | pre control flow: lines 46-54, 66-76, 100-196 | de610553c312b240e4cb804fa7a5ebf708dced963a5ca46f58bb84359fa05f84 |
| `src/utils/huffman_utils.c` at 902bc91 | counting pass and segments, lines 214-251 | 76f5806c72e84054e629052f3d26dc86e1fc7c2ff0446ea7dd1fc28953b954c4 |
| `src/dec/vp8l_dec.c` at parent | `kTableSize` and `enough.c` note 80-102; `table[1 << LENGTHS_TABLE_BITS]` 265; 3-bit read 346; `ReadHuffmanCodes` 440-485 | 2ac47cddd92273f4d823dafd3f4b29d7adfb7da24f2304ebda388309f6a4bcd3 |
| `src/dec/vp8l_dec.c` at 902bc91 | caller after the fix | c5027ed8e7f2532e97dd9446066eca540973738da7c0bc7ce25648221e0092c4 |
| `src/utils/huffman_utils.h` at parent | `HUFFMAN_TABLE_BITS` 8, `LENGTHS_TABLE_BITS` 7 | 5ff3f924892548b9072b6d932e81ca91c69b82b04028b5ac925ef41b20514acf |
| `src/webp/format_constants.h` at parent | `MAX_ALLOWED_CODE_LENGTH` 15, alphabet sizes | d8e96b2d11a9d5138f78f409404340c411ea9ca0ba5e6a6fd1b6af01b13f6d51 |
