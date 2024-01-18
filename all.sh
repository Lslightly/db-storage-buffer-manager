#!/bin/bash

# Generate a list of BUF values
BUF_LIST="1024 4096"
current_buf=8192

while [ $current_buf -le 20000 ]
do
    BUF_LIST="$BUF_LIST $current_buf"
    current_buf=$((current_buf + 4096))
done

# Loop through the BUF values
for BUF in $BUF_LIST
do
    echo "Executing with BUF=$BUF"
    # Run CMake configuration
    cmake -S . -G Ninja -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -DBUF=$BUF

    # Build the project using Ninja
    ./build.sh

    # Run your executable with the specified parameters
    ./build/DBDriver -bench-file test/data-5w-50w-zipf.txt -log-level 6
done
