from boto.dynamodb2.table import Table
from boto.dynamodb2.exceptions import ItemNotFound, ValidationException

execfile('20141589.conf')

myTable = Table('project6-20141589')
while True:
    try:
        s = raw_input('Input word combination (Ctrl+d to exit) : ')
    except EOFError:
        print ''
        break
    ans = 0
    try:
        elem = myTable.get_item(words = s)
        ans = elem['counts']
    except ItemNotFound:
        pass
    except ValidationException:
        print 'You have put empty string. Ignoring input.'
        continue

    print '%s has %d occurences.' % (s, ans)
