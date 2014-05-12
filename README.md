#Details of the installation process.
=========================================
RTool is based on the  LLVM based front-end, which is written on llvm-3.2,
an intelligent scheduler(inspect-0.3, a thread modular parser(SimpleDeclParser) and 
dynamic invariant generation tool(daikon).

#Installation Process
------------------------------------------
Installation process consits of the following major steps.

1. Checkout the git repository.
2. Uncompress and install llvm from the llvm-3.2.src
3. Install the Daikon Pass.
5. Install smtdp.
6. Install inspect.
7. Install Thread Modular Parser.
8. Install Daikon.
9. Modify and install the path in the runner scripts.


## 1. Checkout the git repository
Checkout the git repository into a local folder by using the following command,
git clone `git@github.com:arijitvt/RTool.git`
