# Limitations

- **Counts only.** No table is written, so `PAST_BUDGET` is an abstract
  count of entries past the precomputed bound, not a write.
- **Per-code bound, not the whole allocation.** The model compares one
  code with its alphabet's `enough.c` bound. In the decoder, a group's five
  codes share `kTableSize[ccb]` entries and groups are contiguous. An
  overrun spills into the next group, and it runs past the allocation only
  in the last group and when the earlier codes leave no slack. That
  placement is **not modelled**.
- **The search is not a proof of maximum.** `search.csv` reports the
  largest extents seen over seeded random histograms. The full-code bound
  of 630 / 410 is the upstream `enough.c` figure, cited, not re-derived.
- **Bitstream not modelled.** The model takes code-length histograms
  directly. How a stream encodes them (code-length codes, repeat codes 16
  to 18, `max_symbol`) is not modelled, beyond the 3-bit read of the
  code-length-code lengths used for the control.
- **Two ports of one function.** The C and Python ports are separate
  implementations of the same upstream function, so they confirm the port
  but do not test libwebp itself.
- **Sources:** the commit and its parent only. Later hardening was not
  read.
- This is not a finding. Nothing is attributed beyond the commit and the
  Chromium bug reference.
- **Input domain:** the public API accepts root_bits 1..14, alphabets of
  1..4096 symbols and counts 0..alphabet whose sum is at most the alphabet.
  Anything else returns BAD_INPUT or BAD_LENGTH before any shift. libwebp
  itself only uses root_bits 7 and 8; the wider range is a model choice.
