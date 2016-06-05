from boto.s3.connection import S3Connection
from boto.dynamodb2.table import Table
from boto.dynamodb2.exceptions import ItemNotFound

execfile('20141589.conf')

s3 = S3Connection()
bucket = s3.get_bucket('sp20141589-proj6')
tbl = Table('proj6-20141589')

for f in bucket.list():
    ngram = f.get_contents_as_string().split('\n')
    for elem in ngram:
        elem = elem.split('\t')
        if len(elem) == 2:
            try:
                elem[1] = int(elem[1])
            except ValueError:
                continue
            try:
                item = tbl.get_item(BigramWord = elem[0])
                item['BigramCount'] += elem[1]
            except ItemNotFound:
                tbl.put_item(data = {'BigramWord': elem[0], 'BigramCount': elem[1]})
            
