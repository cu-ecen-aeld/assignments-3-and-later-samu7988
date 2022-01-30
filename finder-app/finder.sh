#!/bin/sh

filesdir=$1 #path to directory
searchstr=$2 #string to search
status=0

if [ -z "$1" ]
then
echo '$1 argument not passed by user'
status=1
fi

if [ -z "$2" ]
then
echo '$2 argument not passed by user'
status=1
fi

if ! [[ -d $filesdir ]] 
then
echo "filesdir:$filesdir does not represent filepath on system"
status=1
fi

num_files=$(find $filesdir -type f  | wc -l)


num_matching_line=$(grep -rnw $filesdir -e $searchstr | wc -l)

echo "The number of files are $num_files and the number of matching lines are $num_matching_line"
exit $status



