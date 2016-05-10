echo SYNC
time for i in {1..30}; do
  python 20141589_sync.py
  rm *.txt
done
echo
echo

for pool in {1..20}; do
  echo ASYNC WITH $pool workers
  time for i in {1..30}; do
    python 20141589_test_async.py $pool
    rm *.txt
  done
  echo
  echo
done
