#!/bin/bash

if [ $# -lt 4 ]; then
  echo "Usage: $0 <ip> <port> <buffer size> <thread pool size>"
  exit 1
fi

path=./tests/scripts/given/

ls $path | while read -r script; do
    echo "Running: $script"
    eval "$path$script $1 $2 $3 $4" > ./tests/results/results_"$script".txt 2>&1
done