from threading import Thread

from boto.s3.connection import S3Connection
from boto.dynamodb2.table import Table
from boto.dynamodb2.exceptions import ItemNotFound, ConditionalCheckFailedException

execfile('20141589.conf')


def writeDB(ngram, idx):
    tbl = Table('project6-20141589')
    ngram = ngram.split('\n')
    cnt = 0

    for elem in ngram:
        elem = elem.split('\t')
        if len(elem) == 2:
            try:
                elem[1] = int(elem[1])
            except ValueError:
                continue

            atom = {'words': elem[0], 'counts': elem[1]}
            try:
                tbl.put_item(data = atom)
            except ConditionalCheckFailedException:
                try:
                    item = tbl.get_item(words = elem[0])
                    item['counts'] += elem[1]
                    item.save()
                except ItemNotFound:
                    print 'Unknown exception raised for (%s, %d)' % (elem[0], elem[1])
            cnt += 1

            if cnt % 10000 == 0:
                print 'Thread %d running (%d)' % (idx, cnt)
    print 'Thread %d finished with %d' % (idx, cnt)


if __name__ == '__main__':
    s3 = S3Connection()
    bucket = s3.get_bucket('sp20141589-proj6')

    idx = 0
    for f in bucket.list():
        Thread(target = writeDB, args=(f.get_contents_as_string(),idx,)).start()
        idx += 1
