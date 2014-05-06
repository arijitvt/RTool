
#LLVM Latest compilation
export PATH=/home/ari/cvs/src/bin:$PATH
LLVM=/home/arijit/Research/RTool/build/Release+Debug+Asserts
LLVM_INST_DIR=$LLVM/bin
export PATH=$LLVM_INST_DIR:$PATH
export llvm_lib=$LLVM_INST_DIR/lib
export LD_LIBRARY_PATH=$llvm_lib:$LD_LIBRARY_PATH
export dk=
export CLASSPATH=/home/ari/cvs/src/SimpleDeclParser/parser.jar:$CLASSPATH
