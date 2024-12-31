#!/bin/bash -e

version=1.0

if [ ! -d "./out" ]; then
    mkdir "./out"
fi

outfile="main"
rmfile="${main}*"
if ls ./out/${rmfile} 1> /dev/null 2>&1; then
    rm ./out/${rmfile}
fi
#g++ -c ./log/log.cpp -o ./out/log.o -O2 -std=c++17
#g++ -c ./main.cpp -o ./out/main.o -O2 -std=c++17 -Wno-builtin-declaration-mismatch
g++ -static ./main.cpp -o ./out/${outfile}_${version}.exe -std=c++17

