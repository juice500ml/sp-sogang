#!/usr/bin/python

import sys

curr_ngram = None
curr_count = 0
ngram = None

for line in sys.stdin:
    ngram, count = line.split('\t', 1)
    try:
        count = int(count)
    except ValueError:
        continue

    if curr_ngram == ngram:
        curr_count += count
    else:
        if curr_ngram:
            print '%s\t%s' % (curr_ngram, curr_count)
        curr_ngram = ngram
        curr_count = count

if curr_ngram:
    print '%s\t%s' % (curr_ngram, curr_count)
