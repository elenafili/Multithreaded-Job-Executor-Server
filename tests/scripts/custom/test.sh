#!/bin/bash

if [ $# -lt 4 ]; then
  echo "Usage: $0 <ip> <port> <buffer size> <thread pool size>"
  exit 1
fi

ssh -f $1 "cd ./system-programming-hw2 ; ./bin/jobExecutorServer "$2"0 $3 $4"
sleep 2

./bin/jobCommander $1 "$2"0 setConcurrency 5
./bin/jobCommander $1 "$2"0 issueJob sleep 5 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 7 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 7 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 5 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 5 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 10 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 11 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 12 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 3 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 14 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 3 > /dev/null &
./bin/jobCommander $1 "$2"0 issueJob sleep 16 > /dev/null &
./bin/jobCommander $1 "$2"0 poll
echo "Sleeping now!"
sleep 5
echo "Just woke up!"
./bin/jobCommander $1 "$2"0 poll
echo "Exiting ..."
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 exit &
./bin/jobCommander $1 "$2"0 poll &
./bin/jobCommander $1 "$2"0 setConcurrency 10 &
./bin/jobCommander $1 "$2"0 issueJob sleep 100 &
./bin/jobCommander $1 "$2"0 stop job_2 