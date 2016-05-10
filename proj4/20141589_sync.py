import requests
from bs4 import BeautifulSoup, Comment
from collections import deque

ROOT_URL = 'http://cspro.sogang.ac.kr/~gr120160213/'

links = set()
bfs_links = deque()

links.add(ROOT_URL + 'index.html')
bfs_links.append(ROOT_URL + 'index.html')
bfs_len = 1
visit_index = 1

url_fp = open('URL.txt', 'w')

while bfs_len != 0:
    for _ in range(bfs_len):
        l = bfs_links.popleft()
        r = requests.get(l)
        if not r.ok:
            continue
        
        url_fp.write(l + '\n')

        soup = BeautifulSoup(r.text, 'html.parser')

        fp = open('Output_%04d.txt' % visit_index, 'w')
        visit_index += 1
        fp.write(''.join([str(i) for i in soup.strings if not isinstance(i, Comment)]))
        fp.close()

        pall = soup.find_all('a')

        for p in pall:
            tmplink = p['href'].strip('/ \t\n\r')
            if not tmplink:
                continue
            if tmplink[0] == '#' or tmplink[0] == '?':
                continue
            if tmplink[0:7] != 'http://':
                tmplink = ROOT_URL + tmplink

            if tmplink in links:
                continue
            links.add(tmplink)
            bfs_links.append(tmplink)
    bfs_len = len(bfs_links)

url_fp.close()
