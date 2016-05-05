import requests
from bs4 import BeautifulSoup
from collections import deque
from multiprocessing import Pool

def crawl(url):
    l = url
    ret = []
    r = requests.get(l)
    if not r.ok:
        return '', []

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
    
    retlink = '\nhttp://cspro.sogang.ac.kr/~gr120160213/' + filename
    
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
        ret.append(tmplink)
    return retlink, ret


links = set()
bfs_links = list()
url_fp = None
pool = Pool()

links.add('http://cspro.sogang.ac.kr/~gr120160213/index.html')
bfs_links.append('http://cspro.sogang.ac.kr/~gr120160213/index.html')

while bfs_links:
    ret_list = pool.map(crawl, bfs_links)

    for ret in ret_list:
        s, ret_links = ret
        
        if not url_fp:
            url_fp = open('URL.txt','w')
            s = s[1:]
        url_fp.write(s)
        
        bfs_links = list()
        for l in ret_links:
            if l and l not in links:
                bfs_links.append(l)
                links.add(l)

if not url_fp:
    url_fp.close()
