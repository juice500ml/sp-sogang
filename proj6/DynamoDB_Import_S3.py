from threading import Thread

from boto.s3.connection import S3Connection
from boto.dynamodb2.table import Table
from boto.dynamodb2.exceptions import ItemNotFound

execfile('20141589.conf')


def writeDB(ngram):
    tbl = Table('project6')
    ngram = ngram.split('\n')

    for elem in ngram:
        elem = elem.split('\t')
        if len(elem) == 2:
            try:
                elem[1] = int(elem[1])
            except ValueError:
                continue
            try:
                item = tbl.get_item(words = elem[0])
                item['counts'] += elem[1]
            except ItemNotFound:
                tbl.put_item(data = {'words': elem[0], 'counts': elem[1]})


if __name__ == '__main__':
    s3 = S3Connection()
    bucket = s3.get_bucket('sp20141589-proj6')
    for f in bucket.list():
        Thread(target = writeDB, args=(f.get_contents_as_string(),)).start()
