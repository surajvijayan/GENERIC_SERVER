#! /bin/sh

i=0
while [ $i -lt 10000 ]
do
	echo "Cnt: $i"
	#./a.out 10.11.22.203 60103 7405SV "Hello from Suraj Vijayan.." 2 1
	./a.out 10.12.65.72 60103 TEST_PLUGIN1 "Hello from Suraj Vijayan.." 2 1
	i=`expr $i + 1`
done
