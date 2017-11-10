#!/bin/sh

if test $# -ne 1
then
    echo "please input your the name of generator"
fi

mkdir -p generate
cd generate
cmake -G "$1" ../..
cd ..
