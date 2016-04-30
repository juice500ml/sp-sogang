import requests
from bs4 import BeautifulSoup
from collections import deque

"""
r = requests.get('http://cspro.sogang.ac.kr/~gr120160213')
soup = BeautifulSoup(r.text, 'html.parser')

pall = soup.find_all('p')
for i in pall:
    print(i.text)
"""

links = set()
bfs_links = deque()

links.add('http://cspro.sogang.ac.kr/~gr120160213/index.html')
bfs_links.append('http://cspro.sogang.ac.kr/~gr120160213/index.html')
bfs_len = 1

url_fp = open('URL.txt', 'w')

while bfs_len != 0:
    for _ in range(bfs_len):
        l = bfs_links.pop()
        r = requests.get(l)
        if not r.ok:
            continue

        filename = ''
        if l[-1] == '/':
            spl = l.rsplit('/', 2)
            l = spl[0]
            filename = spl[1]
        else:
            spl = l.rsplit('/', 1)
            l = spl[0]
            filename = spl[1]
        l += '/'
        
        url_fp.write('http://cspro.sogang.ac.kr/~gr120160213/' + filename + '\n')
        filename = 'Output_' + filename.split('.')[0] + '.txt'
        fp = open(filename, 'w')
        fp.write(r.text)
        fp.close()

        soup = BeautifulSoup(r.text, 'html.parser')
        pall = soup.find_all('a')
        for p in pall:
            tmplink = p['href'].strip('/ \t\n\r')
            if not tmplink:
                continue
            if tmplink[0] == '#' or tmplink[0] == '?':
                continue
            if tmplink[0:7] != 'http://':
                tmplink = l + tmplink
            if tmplink in links:
                continue
            links.add(tmplink)
            bfs_links.append(tmplink)
    bfs_len = len(bfs_links)
    if bfs_len == 0:
        break

url_fp.close()
