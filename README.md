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
  7. Update the path in the exports.sh 


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

## 3. Install smtdp
Since Inspect uses smt solver in the backend, so it has the depency on this. Go to the smt_dp folder and run `make` from there. It will compile and copy the required files in proper location.

##4. Install Inspect
  Installing Inspect is also very straight forward. Go inside the inspect-0.3 folder and run the `make` command from the folder. It will compile and create the inspect executatable inside the RTool/bin folder. To test whether, the `make` command executed properly, check if you have the  inspect executable present in the RTool/bin folder. 
  
  After the installation set `inst` path in the RTool/exports.sh file.

##5. Install Thread Modular Parser
Jar file for the Thread modular parser is present in the RTool/jars. Update the class path with the location of the parser.jar in RTool/exports.sh.

However the source code for that jar file is already present in the RTool/SimpleDeclParser folder and one can create the jar from the java source using standard jar file making process and update the corresponding classpath with the jar location in the export.sh.

##6. Install Daikon
The complete guide for Daikon installation can be found at this [website](http://plse.cs.washington.edu/daikon/download/doc/daikon.html).
However there are some simple steps associated to it.
 - Download and install jdk 1.7 \(jdk 1.6 does not work for daikon\).
 - Update the JAVA_HOME in the bashrc
 - Populate DAIKONDIR variable with the top directory of the daikon folder and export the variable.
 - Export the daikon.bashrc by running `source $DAIKONDIR/scripts/daikon.bashrc`
 - Then run make from the daikon folder. This will install daikon and update all the required the path.

##7. Update the path in the exports.sh 
During the installation process we have already updated various path variables. We may double check all the path variables at this stage. Once that is done, update the `PATH` variable with the RTool/bin and update the `daikon` variable with the source folder where the DaikonPass resides. DaikonPass folder exists in the `llvm-3.2.src/lib/Transforms/Daikon` folder. This folder should contain the hook.h and hooh.c files, which contains the source for the *hook_assert* macro.


#How to use RTool
===========================
RTool can be used in three modes.

1. Use the existing daikon infrastructure through a nitty gritty interface.
2. Use the RTool infrastructure to generate invarint at all function entry-exit and around the global variable access locations.
3. Generate the invariants at various location varied dynamically using command line control.

Before using RTool, check if you can have access to `controller.py`,`runner.py` and `r_tool.py`. If it is not possible to access them then update the path variable properly in the RTool/exports.sh and check again.

##1.  RTool for Daikon as it is
--------------------------------
RTool can be used to run Daikon as it is in a more comprehensive way and using much more shorter commands. To compile the program and dump the program points, run the command 

`controller.py daikon dump`

This compile the program, create the executable and create the program points cum variables as well. One can edit these files, to reduce the number of porgram points and/or variables of interest on which the invariant should be generated.
Once this step is over, then we should run the program, using the command 

`controller.py daikon rep #`

Here <#> should be the run count/iteration count. Since daikon needs four different values of the variables, so we may have edit the source with different values of global variables and run the program multiple times.  That is the reason we have to supply this number and iterate this.  This process will dump all the thread-modular trace files inside a trace folder, called and one can generate invariant using the daikon's backend by running the command,

`java daikon.Daikon *.dtrace`

##2. RTool's Invariant Generation
The main usage of the RTool is to generate invariants for concurrent programs. For that purpose, we should invoke rtool in similar fashion as discussed in the previous section. To compile the program and generate program points, use the below mentioned command.

`controller.py rtool comp`

Once that is generated, we can use

`controller.py rtool run $ # %`

Here $ corresponds to the iteration count (as we mentioned in the previous section about this iteration count). # corresponds to the algo choice. Since inspect can use DPOR, PCB or HaPSet for systematic exploration, so we can dynamically choose the algorithm(0 -> DPOR, 1 -> PCB, 2->HaPSet). % denotes the number, required for PCB and HaPSet. It should  ideally be 0 for DPOR.

##3. RTool for Critical Section Determination
The compilation should follow the exact same procedure as above, except the first argument will be 'cs':

`controller.py cs comp`


To run the program,

`controller.py cs run $ @ # %`

Rest of the three special characters \($,#,%\), means exactly the same as previous section describes, only @ should be replaced with the spacer count, to dynamically vary the locations of instrumentation to determine the cs. 

All these utilities can be ran though a ui built based on qt framework. An sample ui can be seen as,
    ![GitHub Logo](/images/Rui.png)
	Format: ![Alt Text](/images/Rui.png)
