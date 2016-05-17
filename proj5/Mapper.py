import sys

for line in sys.stdin:
    words = line.strip().split()
    count = len(words)
    for i in range(count-1):
        print '%s %s\t1' % (words[i], words[i+1])
