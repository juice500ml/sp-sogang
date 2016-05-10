import requests
from bs4 import BeautifulSoup
from multiprocessing import Pool
import gc

# garbage collection turn off
gc.disable()

# Root url for visiting
ROOT_URL = 'http://cspro.sogang.ac.kr/~gr120160213/'

# crawling function
def crawl(url):
    # ret_links are contained url in "url"
    ret_links = []
    r = requests.get(url)
    if not r.ok:
        return None

    ret_link = url
    # parse "url" with bs4 module
    soup = BeautifulSoup(r.text, 'html.parser')
    ret_text = soup.text

    # find all links in soup
    pall = soup.find_all('a')

    for p in pall:
        # finding href value
        tmplink = p['href'].strip('/ \t\n\r')
        if not tmplink:
            continue
        if tmplink[0] == '#' or tmplink[0] == '?':
            continue
        if tmplink[0:7] != 'http://':
            tmplink = ROOT_URL + tmplink

        # if tmplink is already in links, don't push
        if tmplink in ret_links:
            continue
        ret_links.append(tmplink)

    return ret_link, ret_links, ret_text


# For only main py thread (using multithreading)
if __name__ == '__main__':
    # visited links
    links = set()
    # to-be visited
    bfs_links = list()
    url_fp = open('URL.txt', 'w')
    visit_index = 1
    # Pool size is fixed by experiment
    pool = Pool(6)

    # root link
    links.add(ROOT_URL + 'index.html')
    bfs_links.append(ROOT_URL + 'index.html')

    # bfs traversal
    while bfs_links:
        # multithreading all requests
        ret_list = pool.map(crawl, bfs_links)

        # for all request results
        for ret in ret_list:
            if not ret:
                continue

            ret_link, ret_links, ret_text = ret

            # writing ret_link
            if visit_index != 1:
                ret_link = '\n' + ret_link
            url_fp.write(ret_link)
            with open('Output_%04d.txt' % visit_index, 'w') as fp:
                fp.write(ret_text)
            visit_index += 1

            # for next bfs search, populate bfs_links
            bfs_links = list()
            for l in ret_links:
                if l and l not in links:
                    bfs_links.append(l)
                    links.add(l)
