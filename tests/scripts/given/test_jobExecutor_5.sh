#!/bin/bash

if [ $# -lt 4 ]; then
  echo "Usage: $0 <ip> <port> <buffer size> <thread pool size>"
  exit 1
fi

ssh -f $1 "cd ./system-programming-hw2 ; ./bin/jobExecutorServer "$2"5 $3 $4"
sleep 2

./bin/jobCommander $1 "$2"5 issueJob ./bin/progDelay 5 &
./bin/jobCommander $1 "$2"5 issueJob ./bin/progDelay 4 &
./bin/jobCommander $1 "$2"5 issueJob ./bin/progDelay 3 &
./bin/jobCommander $1 "$2"5 issueJob ./bin/progDelay 2 &
./bin/jobCommander $1 "$2"5 issueJob ./bin/progDelay 1 &
./bin/jobCommander $1 "$2"5 poll
./bin/jobCommander $1 "$2"5 setConcurrency 2
./bin/jobCommander $1 "$2"5 poll
./bin/jobCommander $1 "$2"5 exit