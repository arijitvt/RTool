
#LLVM Latest compilation
export PATH=/home/ari/cvs/src/bin:$PATH
LLVM_INST_DIR=/home/ari/cvs/src/build/Release+Debug+Asserts/bin
export PATH=$LLVM_INST_DIR:$PATH
export llvm_lib=/home/ari/cvs/src/build/Release+Debug+Asserts/lib
export LD_LIBRARY_PATH=$llvm_lib:$LD_LIBRARY_PATH
export dk=/home/ari/cvs/src/llvm-3.2.src/lib/Transforms/Daikon
export CLASSPATH=/home/ari/cvs/src/SimpleDeclParser/parser.jar:$CLASSPATH
