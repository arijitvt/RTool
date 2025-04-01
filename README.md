# Details of the installation process.
=========================================
RTool is a dynamic invariant generation tool for sequential and concurrent C/C++ programs.
It builds upon the LLVM compiler front-end (llvm-3.2), the INSPECT concurrency testing
tool (inspect-0.3), a Java based trace classifier (SimpleDeclParser) and the 
Daikon dynamic invariant generation tool (daikon).

# Installation Process
------------------------------------------
The installation process consits of the following steps.
  1. Checkout the RTool git repository.
  2. Install LLVM and the DaikonPass
  3. Install inspect.
  4. Install the trace classifier SimpleDeclParser.
  5. Install Daikon.  
  6. Update the path in the exports.sh 


## 1. Checkout the RTool git repository
Checkout the git repository into a local folder by using the following command,

`git clone git@github.com:arijitvt/RTool.git`

This will create a folder named RTool, which is mostly a self-contained package, with most of the dependencies. 

## 2. Install LLVM and the DaikonPass
Inside the RTool folder, there exists our customized version of llvm (llvm-3.2.src.tar.bz2) compressed file. In order to install this version of LLVM,
  - First, untar the package by running the command `tar -xvf llvm-3.2.src.tar.bz2`, this will create a folder named llvm-3.2.src and places the source inside the folder.
  - Copy the Daikon folder (which is inside the LLVM_PASS folder) to the existing folder called llvm-3.2.src/lib/Transform.
  - Then, go to the `build` directory, which should contain `conf.sh` file. Change the executable permission of the script by running the command `chmod +x conf.sh`.
  - Run the `conf.sh` from the build folder. This will build LLVM and all its code transformation passes that are needed to run RTool. The resulting files are inside the `build/Release+Debug+Asserts` folder.
  - Finally, update the LLVM variables to have the complete path point to `Release+Debug+Asserts` inside the RTool/exports.sh.

## 3. Install Inspect
Since our customized version of the Inspect tool uses the SMT solver, we need to install smtdp first. Go to the `smt_dp` folder and run `make` from there. It will compile and also copy the required files to the proper location. 

Installing Inspect is also straightforward. Go to the inspect-0.3 folder and run the `make` command from there. It will compile and create the inspect executatable inside the RTool/bin folder. To test whether the `make` command has been executed properly, check if you have the `inspect` executable in the `RTool/bin` folder. 
  
After the installation, remember to set the `insp` path in the RTool/exports.sh file.

## 4. Install the Trace Classifier
The jar file for the trace classifier is present in the `RTool/jars` folder. Update the class path with the location of the `parser.jar` in `RTool/exports.sh`.

Since the source code for that jar file is already present in the `RTool/SimpleDeclParser` folder, if needed, one can also create the jar file again from the java source, using the standard jar file making process, and then update the corresponding classpath with the jar location in the `RTool/exports.sh`.

## 5. Install Daikon
The original guide for Daikon installation can be found at this [website](http://plse.cs.washington.edu/daikon/download/doc/daikon.html).
Below is a brief summary of the major steps associated with this process.
 - Download and install jdk 1.7 \(I've found that jdk 1.6 does not work for the current version of daikon\).
 - Update the JAVA_HOME in the bashrc with the installation directory of JDK1.7.
 - Populate DAIKONDIR variable with the top directory of the daikon folder and export the variable.
 - Export the daikon.bashrc by running `source $DAIKONDIR/scripts/daikon.bashrc`
 - Then run `make` from the daikon folder. This will install daikon and update all the required the path.

## 6. Update the path in the exports.sh 
During the installation process, we have already updated various path variables. You may want to double check if all the path variables are set properly at this stage. 

Once that is done, update the `PATH` variable with the `RTool/bin` and update the `daikon` variable with the source folder where the DaikonPass resides. DaikonPass folder exists in the `llvm-3.2.src/lib/Transforms/Daikon` folder. This folder should contain the `hook.h` and `hook.c` files, which contains the source for the *hook_assert* macro that we have defined.


# How to use RTool
===========================
RTool can be used in three modes.

1. Use the existing daikon infrastructure through a nitty gritty interface.
2. Use the RTool infrastructure to generate invarint at all function entry-exit and around the global variable access locations.
3. Generate the invariants at various location varied dynamically using command line control.

Before using RTool, check if you can have access to the three scripts: `controller.py`,`runner.py` and `r_tool.py`. If it is not possible to access these scripts, then update the path variable properly in the RTool/exports.sh and check again.

## 1.  RTool for Daikon as is
--------------------------------
RTool can be used to run Daikon as is, in a more comprehensive way and using much shorter commands. To compile the target program (the program for which you want to generate invariants) and dump the program points, run the command 

`controller.py daikon dump`

This compiles the target program, creates the executable, and also creates the program points and/or variables of interest as well. You may edit these files, for example, to reduce the number of porgram points and/or variables of interest (on which the invariants are generated).

Once this step is over, then we should be able to run the instrumented target program, using the command 

`controller.py daikon rep #`

Here <#> should be the run count/iteration count -- that is, how many times you want to run the target program before giving the traces to Diakon for invariant generation. 

Since daikon needs at least four different values of the variables for generating linear invariants, you may need to edit the program source so that the program runs under different program inputs (different values of global variables during different runs).  

That is the main reason why we need to supply this iteration count.  This process will also dump all the thread-modular trace files inside a trace folder, called *arijit*, where one can generate invariant using the daikon's backend by running the command from *arijit* folder,

`java daikon.Daikon *.dtrace`

## 2. RTool's Invariant Generation
The main usage of the RTool is to generate invariants for not only sequential C/C++ programs, but also concurrent C/C++ programs. For this purpose, we should invoke RTool in a similar fashion as discussed in the previous section. However, we also need to run the target program under the supervision of the Inspect systematic concurrency testing tool. 

To compile the program for Inspect and at the same time generate program points, use the below mentioned command.

`controller.py rtool comp`

Once the instrumented program is generated, we can use

`controller.py rtool run $ # %`

Here, $ corresponds to the iteration count (as mentioned in the previous section), and # corresponds to the algo choice. Since Inspect can use three strategies (DPOR, PCB or HaPSet) for systematic exploration of the possible thread interleavings, we can use the command-line argument to dynamically choose the algorithm (0 -> DPOR, 1 -> PCB, 2->HaPSet). When we choose PCB or HaPSet, % denotes the number as required for PCB (context bounds) and HaPSet (size of HaPSet vectors). For DPOR, this number should be set to 0.

##3. RTool for Atomic Section Determination
When RTool is used to automatically infer atomic sections in the program code, the compilation should follow the exact same procedure as above, except that the first argument will be 'cs':

`controller.py cs comp`


To run the program,

`controller.py cs run $ @ # %`

The rest of the three special characters \($,#,%\) remain the the same as described in the previous section, except that @ should be replaced with the spacer count, which set the number of global instructions that should be included in the same atomic region, allowing the user to dynamically vary the locations of instrumentation to determine the atomic region. 

All these utilities can be executed though a GUI built based on the `qt` framework. An sample of this GUI can be seen as,
    ![GitHub Logo](/images/Rui.png)

