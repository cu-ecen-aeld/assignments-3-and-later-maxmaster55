#!/bin/bash

filesdir=$1
searchstr=$2

if [[ -z $filesdir || -z $searchstr ]];
then
	exit 1
fi

if [[ ! -d $filesdir ]];
then
	exit 1
fi


nfiles=$(find "$filesdir" -type f | wc -l)
nline=$(find "$filesdir" -type f -exec grep -i -n "$searchstr" {} + | wc -l)




echo "The number of files are $nfiles and the number of matching lines are $nline"


