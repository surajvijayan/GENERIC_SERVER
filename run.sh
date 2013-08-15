#!/bin/sh

find . -name "*.cpp" -o -name "*.h"|
while read LINE
do
	echo "Doing: $LINE"
	cat lic $LINE > "$LINE".one 
	mv "$LINE".one $LINE
done
