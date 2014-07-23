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
#include "llvm/DerivedTypes.h"

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


		//Get Rep Type String
		string getRepTypeString(Value *val);
		string getRepTypeString(Type *ty);

		//Get Dec Type String
		string getDecTypeString(Value *val);
		string getDecTypeString(Type *ty);

		//Type related  function.
		bool isSupportedType(Value *val);
		bool isSupportedType(Type *type);

		//Dump the structure nexted members
		void dumpStructureMembers(fstream &declFile,
				Value *structElement,Type *ty,int tabCount,bool isGlobalStructure);

		//Dump the various different types of Array
		void dumpArrays(fstream &declFile,
				Value *arrayElement, Type *ty,int tabCount,bool isGlobalArray);


		//Get the type of the global variables 
		//as they are by default pointer type 
		Type* getGlobalType(PointerType *ty) ;

		string getArrayElementTypeString(Type *ty);
		Type* getArrayElementType(Type *ty);

	private:
		vector<Value*>  globalList;
		vector<string> 	doNotInstrumentFunctions;
		vector<string> programPoints;
		Type *voidType ;
		IntegerType *int8Type ; 	
		IntegerType *int16Type;
		IntegerType *int32Type;
		IntegerType *int64Type;
		
		Type *floatType;
		Type *doubleType;

		StructType *structType;

		PointerType *ptr8Type ;
		PointerType *ptr16Type;
		PointerType *ptr32Type;
		PointerType *ptr64Type;

		PointerType *ptrFloatType;
		PointerType *ptrDoubleType;

		PointerType *ptrPtr32Type;
		PointerType *ptrPtr64Type;

		PointerType *ptrStructType;

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



//Primitive types
static const string CHAR_TYPE 	= "char";
static const string SHORT_TYPE 	= "short";
static const string INT_TYPE 	= "int";
static const string LONG_TYPE 	= "long";
static const string DOUBLE_TYPE = "double";
static const string FLOAT_TYPE  = "float";

//Nested Types 
static const string STRUCT_TYPE = "struct";

//Sequential Types
static const string ARRAY_TYPE 	= "array";

//Pointer Type
static const string POINTER_TYPE = "pointer";

#endif
