
#LLVM Latest compilation
export PATH=/home/ari/cvs/src/bin:$PATH
export LLVM=/home/arijit/Research/RTool/build/Release+Debug+Asserts
LLVM_INST_DIR=$LLVM/bin
export PATH=$LLVM_INST_DIR:$PATH
export llvm_lib=$LLVM_INST_DIR/lib
export LD_LIBRARY_PATH=$llvm_lib:$LD_LIBRARY_PATH
export CLASSPATH=/home/arijit/Research/RTool/jars/parser.jar:$CLASSPATH
