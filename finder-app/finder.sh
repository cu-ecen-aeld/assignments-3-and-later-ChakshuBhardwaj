#!/bin/bash
filesdir=$1
searchstr=$2
argsentered=$#
argsneeded=2
if [ ${argsentered} -eq ${argsneeded} ]
then
	if [ -d ${filesdir} ] 
	then
		cd ${filesdir}
		numberoffiles=$(find . -type f |  wc -l)
		numberoflines=$(grep -r "${searchstr}" * | wc -l)
		echo The number of files are ${numberoffiles} and the number of matching lines are ${numberoflines}
		exit 0
	else
		echo The entered directory does not exist!!!
		exit 1
	fi
else
	echo Arguments missing, Expected ${argsneeded} but entered ${argsentered}
	exit 1
fi
