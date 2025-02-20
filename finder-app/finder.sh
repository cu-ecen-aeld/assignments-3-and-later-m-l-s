#!/usr/bin/env sh

if [ $# != 2 ]
then
    echo Usage: $0 dirname string
    exit 1
fi

filesdir=$1
searchstr=$2

if ! [ -d $filesdir ]
then 
    echo First argument must be a directory
    exit 1
fi
 
echo -n The number of files are `find $filesdir \( -type f -or -type l \) -print | wc -l`
echo \ and the number of matching lines are `grep -r $searchstr $filesdir  2>/dev/null  | wc -l`
