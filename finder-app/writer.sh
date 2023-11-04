#!/bin/bash
writefile=$1
writestr=$2
argumentsentered=$#
argumentsexpected=2

if [ $argumentsentered -eq  $argumentsexpected ]
then
	dirpath=$(dirname "$writefile")
	filename=$(basename "$writefile")
	mkdir -p $dirpath && cd $dirpath && echo $writestr > $filename
	exit 0	
else
	echo Arguments missing, Expected $argumentsexpected but entered $argumentsentered
	exit 1
fi
