/**
 * \author Arijit Chattopadhyay
 */

#ifndef DAIKON_PASS_HH
#define DAIKON_PASS_HH


#include "llvm/Pass.h"
#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/Constants.h"
#include "llvm/Type.h"
#include "llvm/InstrTypes.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/InstIterator.h"

#include "llvm/Support/CommandLine.h"

#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/InstrTypes.h"
using namespace llvm;

#include "FileInfo.h"


#include <string>
#include <vector>
#include <set>
#include <fstream>
#include <algorithm>
using namespace std;


class DaikonPass:public ModulePass {

	public:
		static char ID;

	public:
		DaikonPass();
		virtual bool runOnModule(Module &module);

	private:
                void hookAtFunctionStart(Function *);
                void hookAtFunctionEnd(Function *);
		void  insertDynamicCallAtGlobalAccess(Function *);
                
		//hookForStore is not in use 
		void hookForStore(Function *);
                
		//Program PointGeneration
		void generateProgramPoints(Module &);
		void loadProgramPoints(Module &);
		void dumpDeclFileAtEntryAndExit(Function*,string,fstream &);
		void dumpDeclFile(Module&);
		// This process will be refined later
		void dumpForHookAfterFunction(fstream&,string,Function *);

		//Utilities
		void populateGlobals(Module&);
		bool doNotInstrument(StringRef);
		string declDumperForGlobals(Value *,bool,Instruction *instr=NULL);
		string declDumperForCSGlobals(vector<string> variableNames, Instruction *firstInstruction, Instruction *lastInstruction, bool EntryOrExit);

		//These are helper methods
		bool isGlobal(Value *);
		void doInit(Module *);
		Value*  getValueForString(StringRef,Module *);
		string getTypeString(Value *v);
		string getTypeString(Type *v);
		void putTabInFile(fstream &,int);

		//This is for testing
		void testMethod(Function *);

		//These are debug functions
		void displayLoadedProgrampoints();
		void displayGlobalVars();


		//Atomic Region Test Methods
		void handleStoreInst(StoreInst *storeInst);
		void blockBreaker(Instruction *);

		//Type upgrade related functions
		bool isSupportedType(Value *val);
		bool isSupportedType(Type *ty);


	private:
		vector<Value*>  globalList;
		vector<string> 	doNotInstrumentFunctions;
		vector<string> programPoints;
		Type *voidType ;
		IntegerType *int8Type ; 	
		IntegerType *int32Type;
		IntegerType *int64Type;
		PointerType *ptr8Type ; 
		PointerType *ptr32Type;
		PointerType *ptr64Type;
 
		PointerType *ptrPtr32Type;
		PointerType *ptrPtr64Type;

		FunctionType *functionType;
		static bool isInit;
		Value *clapDummyVar;

};


class DummyVarInsertionPass:public ModulePass {
	public:
		DummyVarInsertionPass();
		virtual bool runOnModule(Module&);

	private:
		void insertDummyStoreIntoFunction(Function *func);


	public:
		static char ID;

	private:
		GlobalVariable *dummyVar;
};



#endif
