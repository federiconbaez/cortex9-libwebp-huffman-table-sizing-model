"""Independent expectations for fixtures/cases.tsv.

A second, separate port of BuildHuffmanTable's control flow, written in
Python straight from huffman_utils.c (902bc91 and its parent). It does not
call or read the C model. It also checks the Kraft sum with exact fractions
so INCOMPLETE / FULL / OVERSUBSCRIBED are confirmed by a second method.
Usage: derive_expected.py > fixtures/expected.tsv
"""
from fractions import Fraction
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
MAXLEN = 15


def nexttbs(count, length, root):
    left = 1 << (length - root)
    while length < MAXLEN:
        left -= count[length]
        if left <= 0:
            break
        length += 1
        left <<= 1
    return length - root


def nextkey(key, length):
    step = 1 << (length - 1)
    while key & step:
        step >>= 1
    return (key & (step - 1)) + step if step else key


def build(counts, n, root):
    count = [n - sum(counts)] + list(counts)       # count[0] = unused symbols
    if count[0] == n:
        return "ALL_ZERO", 0, 0
    for length in range(1, MAXLEN):
        if count[length] > (1 << length):
            return "BAD_LENGTH", 0, 0
    if sum(counts) == 1:
        return "SINGLE", 1 << root, 1 << root
    total = 1 << root
    key, low, nodes, opn, mask = 0, -1, 1, 1, total - 1
    for length in range(1, MAXLEN + 1):
        opn <<= 1
        nodes += opn
        opn -= count[length]
        if opn < 0:
            return "OVERSUBSCRIBED", total, 0
        while count[length] > 0:
            if length > root and (key & mask) != low:
                total += 1 << nexttbs(count, length, root)
                low = key & mask
            key = nextkey(key, length)
            count[length] -= 1
    kraft = sum(Fraction(c, 1 << (i + 1)) for i, c in enumerate(counts))
    if nodes != 2 * sum(counts) - 1:
        assert kraft < 1, "num_nodes says incomplete; Kraft must agree"
        return "INCOMPLETE", total, 0
    assert kraft == 1, "num_nodes says full; Kraft must agree"
    return "FULL", total, total


def main():
    out = ["name\ttree\tpre_extent\tfull_size"]
    for line in (ROOT / "fixtures/cases.tsv").read_text().splitlines():
        if not line or line.startswith("#"):
            continue
        name, n, root, counts, _ = line.split("\t")
        tree, ext, full = build([int(x) for x in counts.split(",")], int(n), int(root))
        out.append(f"{name}\t{tree}\t{ext}\t{full}")
    sys.stdout.write("\n".join(out) + "\n")


if __name__ == "__main__":
    main()
