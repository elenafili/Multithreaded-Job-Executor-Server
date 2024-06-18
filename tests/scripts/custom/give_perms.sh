#!/bin/bash

directory="./tests/"

find "$directory" -type f -name "*.sh" | while read -r file; do
    chmod u+x "$file"
done
