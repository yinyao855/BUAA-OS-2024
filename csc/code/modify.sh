#!bin/bash

sed ''s/$2/$3/g'' $1 > tmp
rm $1
mv tmp $1
