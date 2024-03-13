#!/bin/bash

mkdir test
cp -r code test/
cat ./code/14.c

path='./test/code/'
cd $path

i=0
while [ $i -le 15 ]
do
    gcc -c "$i.c" 
    let i=i+1
done

gcc *.o -o ../hello

./hello 2> err.txt

mv err.txt ../../

chmod rw-r-xr-x err.txt

line=$(($1+$2))

sed -n '$linep' err.txt >&2 
