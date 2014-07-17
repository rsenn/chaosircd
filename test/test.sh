#!/bin/sh
killall -9 strace >/dev/null
killall -9 servauth >/dev/null
rm -f *.log *.trace
sleep 1
ORDER="test1 test4 test5 test6 test3 test2 test7 test8
       test9 test10 test11 test12 test13 test14"
       #test15
for x in $ORDER
do
  setsid strace -f -s 1024 ../src/ircd -f "../test/${x}.conf" >${x}.trace &
done
