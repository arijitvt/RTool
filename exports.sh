#Bin path setting for the local bin
export PATH=/home/zach/RTool/bin:$PATH

#LLVM Latest compilation
LLVM=/home/zach/RTool/build/Release+Debug+Asserts
export PATH=$LLVM/bin:$PATH
export llvm=$LLVM
export llvm_bin=$LLVM/bin
export llvm_lib=$LLVM/lib
export LD_LIBRARY_PATH=$llvm_lib:$LD_LIBRARY_PATH

#Inspect PathSetting
insp=/home/zach/RTool/inspect-0.3
export insp
export insp_lib=$insp/lib
export insp_bin=$insp/bin

#Daikon Path for hook.h
export daikon=/home/zach/RTool/llvm-3.2.src/lib/Transforms/Daikon

#ClassPath setting for the parser jar
<<<<<<< HEAD
export CLASSPATH=/home/arijit/Research/RTool/jars/parser.jar:$CLASSPATH

#Setting librayr paths
export LD_LIBRARY_PATH=/home/arijit/Research/RTool/smt_dp/lib:$LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/home/arijit/Research/RTool/./yices-1.0.39/lib/libyices.so:$LD_LIBRARY_PATH


#Path setting for Daikon
source $DAIKONDIR/scripts/daikon.bashrc
export CLASSPATH=/home/zach/RTool/jars/parser.jar:$CLASSPATH
