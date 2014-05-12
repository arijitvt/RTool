compiler=clang
input=test.c


$compiler -emit-llvm -S $input -o output.ll
opt -load $llvm_lib/DaikonPass.so -form output.ll -o executable
