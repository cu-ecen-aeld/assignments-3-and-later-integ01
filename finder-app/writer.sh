#!/bin/bash



if [ $# -lt 2 ]
then 
	echo "Error: No FILE DIR or SEARCH string parameters specified"
	exit 1
else
	WRITEFILE=$1
	WRITESTR=$2
fi

DIR="$(dirname "$WRITEFILE")"
if ! [ -d "$DIR" ] 
then
	mkdir -p "$DIR"
	#echo "FIle or directory not exists"
fi

echo "$WRITESTR" > "$WRITEFILE"
exit $?


