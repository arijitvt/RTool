#Details of the installation process.
=========================================
RTool is based on the  LLVM based front-end, which is written on llvm-3.2,
an intelligent scheduler(inspect-0.3, a thread modular parser(SimpleDeclParser) and 
dynamic invariant generation tool(daikon).

#Installation Process
------------------------------------------
Installation process consits of the following major steps.
  1. Checkout the git repository.
  2. Install llvm and DaikonPass
  3. Install smtdp.
  4. Install inspect.
  5. Install Thread Modular Parser.
  6. Install Daikon.  
  7. Modify and install the path in the runner scripts.


## 1. Checkout the git repository
Checkout the git repository into a local folder by using the following command,

git clone `git@github.com:arijitvt/RTool.git`

This will create a folder named RTool, which mostly a self satisfied package, contains most of the dependencies. 

## 2. Install llvm and DaikonPass
Inside the RTool folder, there exists our customized version of llvm as llvm-3.2.src.tar.bz2 compressed file. In order to install this,
  - First untar the package by running the command `tar -xvf llvm-3.2.src.tar.bz2`, this will create a folder llvm-3.2.src and place the source inside that.
  - Copy the Daikon folder(which is inside the LLVM_PASS) folder to llvm-3.2.src/lib/transform.
  - Then go to the build directory. It should conf.sh file. Give that file executable permission, if it is not having that already by running the command `chmod +x conf.sh`.
  - Run the `conf.sh` from the build folder. This will install the llvm source and all the required passes that are needed to run RTool, inside the build/Release+Debug+Asserts.
  - Update the LLVM variables with the complete path to the Release+Debug+Asserts folder.

## Install smtdp
Since Inspect uses smt solver in the backend, so it has the depency on this. Go to the smt_dp folder and run `make` from there. It will compile and copy the required files in proper location.

##Install Inspect
  Installing Inspect is also very straight forward. Go inside the inspect-0.3 folder and run the `make` command from the folder. It will compile and create the inspect executatable inside the RTool/bin folder. To test whether, the `make` command executed properly, check if you have the  inspect executable present in the RTool/bin folder.
