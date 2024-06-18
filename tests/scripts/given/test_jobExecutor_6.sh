#!/bin/bash

if [ $# -lt 4 ]; then
  echo "Usage: $0 <ip> <port> <buffer size> <thread pool size>"
  exit 1
fi

ssh -f $1 "cd ./system-programming-hw2 ; ./bin/jobExecutorServer "$2"6 $3 $4"
sleep 2

./bin/jobCommander $1 "$2"6 issueJob ./bin/progDelay 10 &
./bin/jobCommander $1 "$2"6 issueJob ./bin/progDelay 10 &
./bin/jobCommander $1 "$2"6 issueJob ./bin/progDelay 10 &
./bin/jobCommander $1 "$2"6 issueJob ./bin/progDelay 10 &
./bin/jobCommander $1 "$2"6 issueJob ./bin/progDelay 10 &
./bin/jobCommander $1 "$2"6 issueJob ./bin/progDelay 10 &
sleep 2
./bin/jobCommander $1 "$2"6 setConcurrency 6
./bin/jobCommander $1 "$2"6 poll
./bin/jobCommander $1 "$2"6 stop job_4
./bin/jobCommander $1 "$2"6 stop job_5
./bin/jobCommander $1 "$2"6 poll
./bin/jobCommander $1 "$2"6 exit
