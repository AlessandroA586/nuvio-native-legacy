#!/usr/bin/env python3
"""Compare text/button scale and ordered artwork identities, counts and sizes."""
import statistics
import sys


def read(path):
    text, cards, rects = [], [], []
    with open(path) as stream:
        for line in stream:
            kind, *values = line.split()
            values = [float(x) for x in values]
            if kind == 'T':
                text.append(values[0])
            elif kind == 'C':
                assert len(values) == 4, 'card log lacks frame/texture identity'
                cards.append(values)
            elif kind == 'R':
                rects.append(values)
    return text, cards, rects


def compare(name, base, target, factor):
    t0, c0, r0 = read(base)
    t1, c1, r1 = read(target)
    assert t0 and t1, 'no text drawn'
    ratio = statistics.median(t1) / statistics.median(t0)
    assert abs(ratio - factor) <= .08, f'text height x{ratio:.3f}, expected {factor}'
    if r0 and r1:
        ratio = statistics.median(h for _, h in r1) / statistics.median(h for _, h in r0)
        assert abs(ratio - factor) <= .12, f'button height x{ratio:.3f}'
    assert len(c0) == len(c1), f'artwork count changed: {len(c0)} -> {len(c1)}'
    for before, after in zip(c0, c1):
        assert before[:2] == after[:2], f'artwork frame/texture changed: {before} -> {after}'
        assert all(abs(a - b) <= 2 for a, b in zip(before[2:], after[2:])), f'artwork resized: {before} -> {after}'
    print(f'PASS {name}: text scaled, {len(c0)} matching artwork draws unchanged')


if __name__ == '__main__':
    try:
        compare(*sys.argv[1:4], float(sys.argv[4]))
    except (AssertionError, ValueError) as error:
        sys.exit(f'FAIL {sys.argv[1]}: {error}')
