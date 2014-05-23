compiler=clang
input=test.c
clean='rm -rf'

if [ $1 == 'dump' ]
then
	$compiler -emit-llvm -S $input -o output.ll
	opt -load $llvm_lib/DaikonPass.so -form -loadppt=false -dumpppt=true output.ll > /dev/null

elif [ $1 == 'compile' ]
then
	$compiler -emit-llvm -S $input -o output.ll
	opt -load $llvm_lib/DaikonPass.so -form -loadppt=true output.ll -o executable.bc
	llvm-dis executable.bc -o executable.ll
	$compiler -c -emit-llvm hook.c -o hook.o
	llvm-link hook.o executable.bc -o combine.o 

elif [ $1 == 'run' ]
then
	lli combine.o -pthread

elif [ $1 == 'clean' ]
then
	$($clean *.bc)
	$($clean *.o)
	$($clean *.ll)
	$($clean *.dtrace)
	$($clean *.ppts)
else
	echo "This is an invalid option"
fi
