#Bin path setting for the local bin
export PATH=/home/arijit/Research/RTool/bin:$PATH

#LLVM Latest compilation
LLVM=/home/arijit/Research/RTool/build/Release+Debug+Asserts
export PATH=$LLVM/bin:$PATH
export llvm=$LLVM
export llvm_bin=$LLVM/bin
export llvm_lib=$LLVM/lib
export LD_LIBRARY_PATH=$llvm_lib:$LD_LIBRARY_PATH

#Inspect PathSetting
insp=/home/arijit/Research/RTool/inspect-0.3
export insp
export insp_lib=$insp/lib
export insp_bin=$insp/bin

#Daikon Path for hook.h
export daikon=/home/arijit/Research/RTool/llvm-3.2.src/lib/Transforms/Daikon

#ClassPath setting for the parser jar
export CLASSPATH=/home/arijit/Research/RTool/jars/parser.jar:$CLASSPATH
