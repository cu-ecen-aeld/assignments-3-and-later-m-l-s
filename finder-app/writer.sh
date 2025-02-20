#!/usr/bin/env bash

if [ $# != 2 ]
then
    echo Usage: $0 filename text_string
    exit 1
fi

writefile=$1
writestr=$2
dir=`dirname $writefile`

if [ -f $dir ]
then
    echo $0: $dir exists and is a file
    exit 1
fi

if ! [ -d $dir ]
then
    mkdir -p $dir 2>/dev/null
    if [ $? != 0 ]
    then
        echo $0: unable to create directory $dir
        exit 1
    fi
fi

echo $writestr >$writefile
