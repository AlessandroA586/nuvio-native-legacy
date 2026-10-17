#!/usr/bin/env python3
"""Fail closed: every named frame must be stable and identical in both trees."""
import sys
from collections import defaultdict


def read(path):
    frames = defaultdict(list)
    with open(path) as stream:
        for line in stream:
            digest, name = line.split()
            frames[name].append(digest)
    return frames


def compare(base, target):
    a, b = read(base), read(target)
    assert a and b, 'no captured frames'
    assert a.keys() == b.keys(), 'missing/extra captured frames'
    for name in a:
        assert len(a[name]) == len(b[name]) == 3, f'{name}: expected three captures per tree'
        assert len(set(a[name])) == len(set(b[name])) == 1, f'{name}: unstable frame; pixel identity not established'
        assert a[name][0] == b[name][0], f'{name}: pixels differ'
    print(f'PASS default 100%: {len(a)} stable frames identical')


if __name__ == '__main__':
    try:
        compare(*sys.argv[1:])
    except AssertionError as error:
        sys.exit(f'FAIL default 100%: {error}')
