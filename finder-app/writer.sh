#!/bin/bash



if [ $# -lt 2 ]
then 
	echo "Error: No FILE DIR or SEARCH string parameters specified"
	exit 1
else
	WRITEFILE=$1
	WRITESTR=$2
fi

if ! [ -e "$WRITEFILE" ] 
then
	#echo "FIle or directory not exists"
	install -Dv /dev/null $WRITEFILE >> /dev/null
fi

echo "$WRITESTR" > "$WRITEFILE"
exit $?


