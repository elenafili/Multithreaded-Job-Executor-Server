#!/bin/bash

if [ $# -lt 4 ]; then
  echo "Usage: $0 <ip> <port> <buffer size> <thread pool size>"
  exit 1
fi

ssh -f $1 "cd ./system-programming-hw2 ; ./bin/jobExecutorServer "$2"3 $3 $4"
sleep 2

./bin/jobCommander $1 "$2"3 issueJob ls -l /usr/bin/* /usr/local/bin/* /bin/* /sbin/* /opt/* /etc/* /usr/sbin/*
./bin/jobCommander $1 "$2"3 exit
