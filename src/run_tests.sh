#!/bin/bash

# Compile
make clean
make

# Read test commands
while read -r line; do
    if [[ ! -z "$line" && ! "$line" =~ ^# ]]; then
        echo "$line" | ./rush
    fi
done < test_cases.txt