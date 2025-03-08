#!/bin/bash



if [ $# -lt 2 ]
then 
	echo "Error: No FILE DIR or SEARCH string parameters specified"
	exit 1
else
	FILESDIR=$1
	SEARCHSTR=$2
fi

if [ -d "$FILESDIR" ]
then
	X="$(find "$FILESDIR" | wc -l)"
	X=$((X-1))
	Y="$(grep -r $SEARCHSTR $FILESDIR | wc -l)"
	echo "The number of files are $X and the number of matching lines are $Y"
else
	echo "Directory does not exist error!"
	exit 1
fi


