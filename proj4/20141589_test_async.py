import requests
from bs4 import BeautifulSoup, Comment
from multiprocessing import Pool
import argparse
import gc

gc.disable()

parser = argparse.ArgumentParser()
parser.add_argument('pool_size', help='Size of process pool')
args = parser.parse_args()

ROOT_URL = 'http://cspro.sogang.ac.kr/~gr120160213/'

def crawl(url):
    ret_links = []
    r = requests.get(url)
    if not r.ok:
        return None

    ret_link = url
    soup = BeautifulSoup(r.text, 'html.parser')
    ret_text = ''.join([str(i) for i in soup.strings if not isinstance(i, Comment)])

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
        ret_links.append(tmplink)

    return ret_link, ret_links, ret_text


if __name__ == '__main__':
    try:
        args.pool_size = int(args.pool_size)
    except:
        print 'Invalid pool size. Pool size has to be integer.'
        print 'Set default to 1'
        args.pool_size = 1

    links = set()
    bfs_links = list()
    url_fp = open('URL.txt', 'w')
    visit_index = 0
    pool = Pool(args.pool_size)

    links.add(ROOT_URL + 'index.html')
    bfs_links.append(ROOT_URL + 'index.html')

    while bfs_links:
        ret_list = pool.map(crawl, bfs_links)

        for ret in ret_list:
            if not ret:
                continue

            ret_link, ret_links, ret_text = ret

            if visit_index != 0:
                ret_link = '\n' + ret_link
            url_fp.write(ret_link)
            with open('Output_%04d.txt' % visit_index, 'w') as fp:
                fp.write(ret_text)
            visit_index += 1

            bfs_links = list()
            for l in ret_links:
                if l and l not in links:
                    bfs_links.append(l)
                    links.add(l)
