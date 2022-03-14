#!/bin/sh

writefile=$1
writestr=$2


if [ -z "$1" ]
then
echo '$1 argument not passed by user'
exit 1
fi

if [ -z "$2" ]
then
echo '$2 argument not passed by user'
exit 1
fi


touch $writefile

if ! [[ -f $writefile ]] 
then
echo "Filename $writefile does not exist"
exit 1
fi

echo $writestr > $writefile

