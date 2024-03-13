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

cd ../

./hello 2> err.txt

mv err.txt ../

cd ../

chmod u+rw-r-xr-x err.txt

line=2

if [ $# -eq 0 ]
then
	line=2
elif [ $# -eq 1 ]
then
	line=$(($1+1))
else
	line=$(($1+$2))
fi

sed -n "$line p" err.txt >&2 

#chmod u+rw-r-xr-x err.txt
