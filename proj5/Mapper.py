#!/usr/bin/python

import sys

for line in sys.stdin:
    words = line.strip().lower().split()
    count = len(words)
    for i in range(count-1):
        print '%s %s\t1' % (words[i], words[i+1])
