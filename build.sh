#!/bin/bash -e

version=1.0

if [ ! -d "./out" ]; then
    mkdir "./out"
fi

outfile="serialization_test"
rmfile="${outfile}*"
if ls ./out/${rmfile} 1> /dev/null 2>&1; then
    rm ./out/${rmfile}
fi

#g++ -c ./src/json.cpp -o ./out/src_json.o -O2 -std=c++17
#g++ -c ./main.cpp -o ./out/main.o -O2 -std=c++17
g++ -static main.cpp  -o ./out/${outfile}_${version}.exe -O2 -std=c++17
