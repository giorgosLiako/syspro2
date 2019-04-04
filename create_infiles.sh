#!/bin/bash



if [ "$#" -le 3 ]
then
    echo "Expected more arguments."
    exit -1
fi

if [ "$2" -lt 0 ]
then
    echo "Number of files is not valid."
    exit -2
fi

if [ "$3" -lt 0 ]
then
    echo "Number of directories is not valid."
    exit -3
fi

if [ "$4" -lt 0 ]
then
    echo "Number of levels is not valid."
    exit -4
fi

if [ ! -d  "$1" ]
then
    mkdir -p $1
fi

gcc -o rand random.c

var1="$(./rand)"

echo "$var1"
