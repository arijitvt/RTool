/**
 * \author Arijit Chattopadhyay
 */

#include "DaikonPass.h"

static cl::opt<bool> dumpppt("dumpppt",cl::init(false),cl::Hidden,cl::desc("This is the dumpppt variable desription"));
static cl::opt<bool> storeinst("storeinst",cl::init(false),cl::Hidden,cl::desc("Store instruction points"));
static cl::opt<bool> load("loadppt",cl::init(false),cl::Hidden,cl::desc("Load Program points"));
static cl::opt<int>  space("spacer",cl::init(-1),cl::Hidden,cl::desc("Spacing the distance between"));

static int counter = 0 ;

DaikonPass::DaikonPass():ModulePass(ID) { 
	doNotInstrumentFunctions.push_back("clap_hookBefore");
	doNotInstrumentFunctions.push_back("hookAfter");
	doNotInstrumentFunctions.push_back("clap_hookFuncBegin");
	doNotInstrumentFunctions.push_back("clap_hookFuncEnd");
	doNotInstrumentFunctions.push_back("clap_chcHook");
	doNotInstrumentFunctions.push_back("clap_chcHookDynamic");

}


bool DaikonPass::runOnModule(Module &module) {

        if(dumpppt && load) {
        	errs()<<"Both dumpppt and load can not be true at the same time \n";
        	return false;
        }
        if(dumpppt) {
        	generateProgramPoints(module);
        	errs()<<"Program Points are being generated\n";
        	return false;
        }
        if(load) {
        	loadProgramPoints(module);
        	errs()<<"Program points are loaded "<<programPoints.size()<<"\n";
        }
        populateGlobals(module);
	//displayGlobalVars();
        if(load) {
        	for(Module::iterator funcItr = module.begin(); funcItr != module.end(); ++funcItr) {
        		Function *func = &*funcItr;
        		string funcName(func->getName().trim().data());
        		if(find(programPoints.begin(), programPoints.end(),funcName) != programPoints.end()) {
        			//Temporarily stopping the hook for Store option
        			hookAtFunctionStart(func);
        			hookAtFunctionEnd(func);
        			//Do not call insertDynamicCallAtGlobalAccess here.
        			//It will jeoperdize the instrumentation.
        		}
        	}
        }
        dumpDeclFile(module);
        //Insert the dynamic fake functions for the global variables
	//Temporarily closing this section
        //if(load) {

        //	for(Module::iterator funcItr = module.begin(); funcItr != module.end(); ++funcItr) {
        //		Function *func = &*funcItr;
        //		string funcName(func->getName().trim().data());
        //		if(find(programPoints.begin(), programPoints.end(),funcName) != programPoints.end()) {
        //			insertDynamicCallAtGlobalAccess(func);
        //		}
        //	}
        //}
        return true;
}



/**
 * This function is openedup to test the type setting.

bool DaikonPass::runOnModule(Module &module) {

        if(dumpppt && load) {
        	errs()<<"Both dumpppt and load can not be true at the same time \n";
        	return false;
        }
        if(dumpppt) {
        	generateProgramPoints(module);
        	errs()<<"Program Points are being generated\n";
        	return false;
        }
        if(load) {
        	loadProgramPoints(module);
        	errs()<<"Program points are loaded "<<programPoints.size()<<"\n";
        }
        populateGlobals(module);
        dumpDeclFile(module);
        return true;
}

**/


/**
 * Private Methods These methods are all helper methods 
 * */
void DaikonPass::populateGlobals(Module &module) {
	globalList.clear();
	for(Module::global_iterator globalItr = module.global_begin(); globalItr != module.global_end(); ++globalItr) {
		GlobalVariable *globalVar = &*globalItr;
		//errs()<<"Global Variable name : "<<globalVar->getName()<<"\n";
		if(globalVar->hasInitializer()) {
			if(globalVar->hasName() && globalVar->getName().equals("__clapDummyGlobalVar")) {
				clapDummyVar = dyn_cast<Value>(globalVar);
				assert(clapDummyVar != NULL);			
				continue;
			}
			string type1 = getTypeString(globalVar->getInitializer()->getType());
			string type2 = getTypeString(globalVar->getType());
			globalList.push_back(globalVar);
		}

	}
} 


bool DaikonPass::isGlobal(Value *value) {
	for(vector<Value*>::iterator itr = globalList.begin(); itr != globalList.end() ; ++itr) {
		Value *v = *itr;
		if(value == v) { 
			return true;
		}
	}
	return false;
}

Value* DaikonPass::getValueForString(StringRef variableName,Module *module) {
	Constant *valueName = ConstantDataArray::getString(module->getContext(), variableName,true);
	Value *val = new GlobalVariable(*module,valueName->getType(), true, GlobalValue::InternalLinkage,valueName);
	return val;
}


void DaikonPass::doInit(Module *module) {
	
	voidType = Type::getVoidTy(module->getContext());
	int8Type  = IntegerType::get(module->getContext(),8);
	int16Type  = IntegerType::get(module->getContext(),16);
	int32Type = IntegerType::get(module->getContext(),32);
	int64Type = IntegerType::get(module->getContext(),64);

	floatType = Type::getFloatTy(module->getContext());
	doubleType = Type::getDoubleTy(module->getContext());

	structType = StructType::create(module->getContext());
	
	ptr8Type  = PointerType::get(int8Type,0);
	ptr16Type = PointerType::get(int16Type,0);
	ptr32Type = PointerType::get(int32Type,0);
	ptr64Type = PointerType::get(int64Type,0);

	ptrFloatType = PointerType::get(floatType,0);
	ptrDoubleType = PointerType::get(doubleType,0);


	ptrPtr32Type = PointerType::get(ptr32Type,0);
	ptrPtr64Type = PointerType::get(ptr64Type,0);

	ptrStructType = PointerType::get(structType,0);

	functionType = FunctionType::get(voidType,true);
	isInit = true;
}

bool DaikonPass::doNotInstrument(StringRef funcName) {
	string funcNameStr(funcName.data());
	for(vector<string>::iterator itr = doNotInstrumentFunctions.begin(); itr != doNotInstrumentFunctions.end();++itr) {
		string s = *itr;
		if(s == funcNameStr) 
			return true;
		if(funcName.startswith("clap_"))
			return true;
	}
	return false;
}

string DaikonPass::getTypeString(Value *value) {
        return getTypeString(value->getType());
}

string DaikonPass::getTypeString(Type *type) {
	switch(type->getTypeID()) {
		case Type::IntegerTyID:{
					       IntegerType *intType=static_cast<IntegerType*>(type);
					       int bitWidth = intType->getBitWidth();
					       if(bitWidth == 8) {
						       return CHAR_TYPE;
					       }else if(bitWidth == 16) {
					       		return SHORT_TYPE;
					       }else if(bitWidth == 32){
						       return INT_TYPE;
					       }else if(bitWidth == 64) {
					       		return LONG_TYPE;
					       }else {
						       return INT_TYPE;
					       }
				       }

		case Type::FloatTyID:{
					     return FLOAT_TYPE;
				     }

		case Type::DoubleTyID:  {
						return DOUBLE_TYPE;
					}

		case Type::StructTyID: {
					       return STRUCT_TYPE;
				       }

		case Type::VectorTyID: {
					       return "vector";
				       }

		case Type::ArrayTyID: {
					      return ARRAY_TYPE;

				      }
		case Type::PointerTyID:{
					       PointerType *ptrType = static_cast<PointerType*>(type);
					       if(ptrType  ==  ptr32Type || ptrType == ptr64Type) {
						       return "int*";
					       }else if(ptrType == ptr8Type) {
						       return "char*";
					       }
					       return POINTER_TYPE;
				       }
		default:  {
				  return "invalid";
			  }

	}
	return "unknown";
}


bool DaikonPass::isSupportedType(Value *val) {
	return isSupportedType(val->getType());
}

bool DaikonPass::isSupportedType(Type *type) {
	switch (type->getTypeID()) {

		case Type::IntegerTyID:
		case Type::FloatTyID:
		case Type::DoubleTyID:  
		case Type::StructTyID: 
		case Type::ArrayTyID: 
			// case Type::VectorTyID: 
			return true;

		default:
			return false;
	}

}


void DaikonPass::putTabInFile(fstream &stream, int tabCount) {
	if(stream.is_open()) {
		for(int i = 0 ; i  < tabCount; ++i) {
			stream<<" ";
		}
	}
}


/**
 * This Function will hook at the beginning of every Function
 */
void DaikonPass::hookAtFunctionStart(Function *func) {
	if(doNotInstrument(func->getName())) return;
        //Do the initial Starts
	Module *module = func->getParent();
	if(!isInit) {
		doInit(module);
	}

	//Now rest of the work

	vector<Value*> intArguments;
	/**
	 * We will Handle only Integer types and ignore all others for time begin
	 */
	for(Function::arg_iterator argItr = func->arg_begin(); argItr != func->arg_end(); ++argItr) {
		Argument *arg = &*argItr;
		Value *val = static_cast<Value*>(arg);
		if(isSupportedType(val)) {
			intArguments.push_back(val);
		}
	}

	int totalArgumentSize = intArguments.size()+globalList.size();
	/**
	 * So far the format is varcount,function name, then globals, function params
	 * The var count will count globals and function params but not the function name
	 */

	Function *hookFunctionBegin = cast<Function>(module->getOrInsertFunction("clap_hookFuncBegin",functionType));
	vector<Value*> argList;
	Constant *argCount = ConstantInt::get(module->getContext(),APInt(32,totalArgumentSize,true));
	argList.push_back(argCount);

	//Send the function name
	string funcNameStr = func->getName().str();
	string finalNameToSend=".."+funcNameStr+"():::ENTER";
	StringRef finalNameToSendRef (finalNameToSend.c_str());
	Value *funcNameValue = getValueForString(finalNameToSendRef,module);
	argList.push_back(funcNameValue);
	
	/**
	 * The format is  varname -vartype-varvalue
	 */
	//First Send the global values
	//We don't have to check the type of global value because
	//We are inserting only integer global parameters
	//So they will always be int.
	for(vector<Value*>::iterator globalItr = globalList.begin(); globalItr != globalList.end() ; ++globalItr) {
		Value *val = *globalItr;
		GlobalVariable *gVal = static_cast<GlobalVariable*>(val);
		string valNameStr = "::"+val->getName().str();
                Value *valName = getValueForString(StringRef(valNameStr.c_str()),module);
	        Value *type=getValueForString(StringRef(getTypeString(gVal->getInitializer()->getType()).c_str()).trim(),module);
		//Value *type=getValueForString(StringRef("int"),module);
		argList.push_back(valName);
		argList.push_back(type);
		argList.push_back(gVal);

	}

	//Now Send the parameters
	for(vector<Value*>::iterator intArgItr = intArguments.begin(); intArgItr != intArguments.end(); ++intArgItr) {
		Value *val= *intArgItr;
		Value *valName = getValueForString(val->getName(),module);
		string typeStr = getTypeString(val);
		Value *type=getValueForString(StringRef(typeStr.c_str()).trim(),module);
		//Value *type=getValueForString(StringRef("int"),module);
		argList.push_back(valName);
		argList.push_back(type);
		argList.push_back(val);
	}
	

#if 0	
        Instruction *target;
        for(inst_iterator institr = inst_begin(func); institr!= inst_end(func); ++institr) {
        	Instruction *ii = &*institr;
        	if(!isa<AllocaInst>(ii)) {
        		target = ii;
        		break;
        	}
        }
        Instruction *hookFunctionBeginInstruction = CallInst::Create(hookFunctionBegin,argList);
	hookFunctionBeginInstruction->insertBefore(target);

	//Insert the dummy load instruction for the inspect instrmentation
    //    LoadInst *loadInst = new LoadInst(clapDummyVar,"clapDummyInstruction",hookFunctionBeginInstruction);
	GlobalVariable *gVar = dyn_cast<GlobalVariable>(clapDummyVar);
       // errs()<<"Init Value : "<<*(gVar->getInitializer())<<"\n";
	//StoreInst *storeInst = new StoreInst(gVar->getInitializer(),gVar,hookFunctionBeginInstruction);
#endif


	Instruction *target = NULL;
        for(inst_iterator instItr = inst_begin(func); instItr != inst_end(func); ++instItr) {
		if(StoreInst *globSt = dyn_cast<StoreInst>(&*instItr)) {
			Value *ptrOp = globSt->getPointerOperand();
			if(ptrOp && ptrOp == clapDummyVar) {
				++instItr;
				if(CallInst *callInst = dyn_cast<CallInst>(&*instItr)) {
					if(callInst->getCalledFunction()->getName().trim().equals("clap_store_post")) {
						++instItr;
						if(instItr != inst_end(func)) {
							target = &*instItr;
							break;
						}
					}
				}
			}
		
		}
	
	}

	//To disable this instrumentation and follow the old approach then the comment out
	//the function call to insertDummyStoreIntoFunction
	Instruction *hookFunctionBeginInstruction = CallInst::Create(hookFunctionBegin,argList);
	if(target != NULL) {
		hookFunctionBeginInstruction->insertBefore(target);
	}else {
		//This is for old approach where the global clap variable is not inserted.
		for(inst_iterator institr = inst_begin(func); institr!= inst_end(func); ++institr) {
			Instruction *ii = &*institr;
			if(!isa<AllocaInst>(ii)) {
				target = ii;
				break;
			}
		}
		hookFunctionBeginInstruction->insertBefore(target);
	}
}



void DaikonPass::hookAtFunctionEnd(Function *func) {
	if(doNotInstrument(func->getName())) return;
        //Formalities first
	Module *module = func->getParent();
	if(!isInit) {
		doInit(module);
	}

	//Work second.
	vector<Value*> intArguments;
	/**
	 * We will Handle only Integer types and ignore all others for time begin
	 */
	for(Function::arg_iterator argItr = func->arg_begin(); argItr != func->arg_end(); ++argItr) {
		Argument *arg = &*argItr;
		Value *val = static_cast<Value*>(arg);
		if(!isSupportedType(val)) {
			intArguments.push_back(val);
		}
	}

	int totalArgumentSize = intArguments.size()+globalList.size();

	
	/**
	 * So far the format is varcount,function name, then globals, function params
	 * The var count will count globals and function params but not the function name
	 */

	//First Check the return type
	bool hasIntReturn = false;
	StringRef returnTypeRef( getTypeString(func->getReturnType()));
	if(returnTypeRef.equals("int")) {
		hasIntReturn = true;
	}

	//Now standard process
	Function *hookFunctionBegin = cast<Function>(module->getOrInsertFunction("clap_hookFuncEnd",functionType));
	vector<Value*> argList;
	Constant *argCount ; 
	if(hasIntReturn) {
		//One extra for return type
		argCount = ConstantInt::get(module->getContext(),APInt(32,totalArgumentSize+1,true));
	}else {
		argCount = ConstantInt::get(module->getContext(),APInt(32,totalArgumentSize,true));
	}

	argList.push_back(argCount);

	//Send the function name
	string funcNameStr = func->getName().str();
	string finalNameToSend=".."+funcNameStr+"():::EXIT0";
	StringRef finalNameToSendRef (finalNameToSend.c_str());
	Value *funcNameValue = getValueForString(finalNameToSendRef,module);
	argList.push_back(funcNameValue);
	
	/**
	 * The format is  varname -vartype-varvalue
	 */
	//First Send the global values
	//Please see the documentation in the hookAtFunctionStart
	//Methods. Detail information is available there.
	for(vector<Value*>::iterator globalItr = globalList.begin(); globalItr != globalList.end() ; ++globalItr) {
		Value *val = *globalItr;
		GlobalVariable *gVal = static_cast<GlobalVariable*>(val);
		string valNameStr = "::"+val->getName().str();
                Value *valName = getValueForString(StringRef(valNameStr.c_str()),module);
		Value *type=getValueForString(StringRef(getTypeString(gVal->getInitializer()->getType()).c_str()).trim(),module);
		//Value *type=getValueForString(StringRef("int"),module);
		argList.push_back(valName);
		argList.push_back(type);
		argList.push_back(gVal);

	}
	//Now Send the parameters
	//for(Function::arg_iterator argItr = func->arg_begin(); argItr != func->arg_end(); ++argItr) {
	for(vector<Value*>::iterator intArgItr = intArguments.begin(); intArgItr != intArguments.end(); ++intArgItr) {
		Value *val= *intArgItr;
		Value *valName = getValueForString(val->getName(),module);
		string typeStr = getTypeString(val);
		Value *type=getValueForString(StringRef(typeStr.c_str()).trim(),module);
		//Value *type=getValueForString(StringRef("int"),module);
		argList.push_back(valName);
		argList.push_back(type);
		argList.push_back(val);
	}

	//First find the return instruction and push the return value if the return is int type
	Instruction *target;
	for(Function::iterator bbItr = func->begin(); bbItr != func->end(); ++bbItr) {
		Instruction *ii = bbItr->getTerminator();
		if(isa<ReturnInst>(ii)) {
			target= ii;
			break;
		}
	}

	if(hasIntReturn) {
		ReturnInst *retInst = static_cast<ReturnInst*>(target);
		Value *val= retInst->getReturnValue();
		Value *valName = getValueForString("return",module);
		Value *type=getValueForString(StringRef(getTypeString(val).c_str()).trim(),module);
		argList.push_back(valName);
		argList.push_back(type);
		argList.push_back(val);
	}

	Instruction *hookFunctionBeginInstruction = CallInst::Create(hookFunctionBegin,argList);
	hookFunctionBeginInstruction->insertBefore(target);
}

void DaikonPass::generateProgramPoints(Module &module) {
	fstream pptFile("ProgramPoints.ppts",ios::out|ios::app);
	if(pptFile.is_open()) { 		
		string modName = module.getModuleIdentifier();
		errs()<<"Module_Name : "<<modName<<"\n";
		pptFile<<"Module_Name : " <<modName<<"\n";
		for(Module::iterator funcItr = module.begin(); funcItr != module.end(); ++funcItr) {
			string functionName (funcItr->getName());
			if(doNotInstrument(funcItr->getName())) {
					continue;
			}
			Function *func= &*funcItr;
			if(func->size() == 0 ) {
				continue;
			}
			functionName += " \n";
			pptFile<<functionName;
		}
		pptFile<<"\n";
		//pptFile<<"hookAfter";
		pptFile.close();
	}
}

void DaikonPass::loadProgramPoints(Module &module) {
	if(!isInit) {
		doInit(&module);
	}

	string moduleName = module.getModuleIdentifier();
	StringRef moduleNameRef(moduleName.c_str());
	moduleNameRef = moduleNameRef.split('.').first.trim();
	ifstream pptFile("ProgramPoints.ppts",ios::in);
	if(pptFile.is_open()){
		string line;
		while(getline(pptFile,line)) {
			StringRef lineRef(line.c_str());
			if(lineRef.startswith("Module_Name")) {
				StringRef modName = lineRef.split(':').second.trim().split('.').first.trim();
				if(modName.equals(moduleNameRef.trim())) {					
					break;
				}
			}
		}

		while(getline(pptFile,line)){			
			StringRef lineRef(line.c_str());
			lineRef = lineRef.trim();		
			//Load only the program points that are related to this module
			if(lineRef.startswith("Module_Name")) {
				break;
			}
                         if( lineRef.size() != 0) {
			 	programPoints.push_back(lineRef.str());
			 }

		}
		pptFile.close();
	}
}

/**
 * Debug function
 * */
void DaikonPass::displayLoadedProgrampoints() {
	for(string s : programPoints) {
		errs() <<"Loaded Program point is : "<<StringRef(s)<<"\n";
	}
}


/**
 * This function returns the rep-type of a type               
 */
string DaikonPass::getRepTypeString(Type *ty) {
	string returnString  = "";
	returnString = getTypeString(ty);
	if(returnString == LONG_TYPE) {
		return INT_TYPE;
	} else if( returnString == CHAR_TYPE) {
		return INT_TYPE;
	} else if( returnString == SHORT_TYPE) {
		return INT_TYPE;
	} else if (returnString == FLOAT_TYPE) {
		return DOUBLE_TYPE;
	}
	//errs()<<"Returning  " <<returnString<<"\n";
	return returnString;
}

/**
 * This function returns the rep-type of a value
 */

string DaikonPass::getRepTypeString(Value *val) {
	Type *ty = val->getType();
	return getRepTypeString(ty);
}

/**
 * This is another test function
 */

Type*  DaikonPass::getGlobalType(PointerType *ty) {
	return ty->getContainedType(0);
//	if(ty == ptrStructType) {
//		return structType;
//	}else if(ty = ptr8Type) {
//		return int8Type;
//	}else if(ty == ptr16Type) {
//		return int16Type;
//	}else if(ty == ptr32Type) {
//		return int32Type;
//	}else if(ty == ptr64Type) {
//		return int64Type;
//	}else if(ty == ptrFloatType) {
//		return floatType;
//	}else if (ty == ptrDoubleType) {
//		return doubleType;
//	}
//	return NULL;                                                                                                                                
}

/**
 * This function returns the dec-type of a type                           
 */
string DaikonPass::getDecTypeString(Type *ty) {
	return getTypeString(ty);
}

/**
 * This function returns the dec-type of a value
 */

string DaikonPass::getDecTypeString(Value *val) {
	Type *ty = val->getType();
	return getDecTypeString(ty);
}

//Handle Structure members                                                                                 
void DaikonPass::dumpStructureMembers(fstream &declFile,
		Value *structElement,Type *ty,int tabCount,bool isGlobalStructure) {
	string structCommonVarName = "var";                                                            
        if(StructType *structType = dyn_cast<StructType>(ty)) {
		StringRef structVariableName  = structElement->getName();
		int counter = 0; 
		for(Type::subtype_iterator eleItr = structType->element_begin() ;
				eleItr != structType->element_end(); ++eleItr,++counter) {
			string elementName = structVariableName.trim().str()+"."+structCommonVarName+"_"+to_string(counter);
			Type *elementType = *eleItr;
			string elementRepType = getRepTypeString(elementType);
			string elementDecType = getDecTypeString(elementType);
			
			putTabInFile(declFile,tabCount);
			if(isGlobalStructure) {
			declFile<<"variable ::"<<elementName<<"\n";
			}else {
				declFile<<"variable "<<elementName<<"\n";
			}
			tabCount =2;
			putTabInFile(declFile,tabCount);
			declFile<<"var-kind field "<<elementName<<"\n";
			putTabInFile(declFile,tabCount);
			declFile<<"rep-type "<<elementRepType<<"\n";;
			putTabInFile(declFile,tabCount);
			declFile<<"dec-type "<<elementDecType<<"\n";
			//if(!isGlobalStructure) {
			//	putTabInFile(declFile,tabCount);
			//	declFile<<"flags is_param\n";
			//}
		}
	}
}


/**
 * Get the element type-string for the
 * nested array.
 */
string DaikonPass::getArrayElementTypeString(Type *ty) {
	if(ty->isArrayTy()) {
		ArrayType *arrTy = dyn_cast<ArrayType>(ty);
		return getArrayElementTypeString(arrTy->getElementType());
	}
	return getTypeString(ty);
}


Type* DaikonPass::getArrayElementType(Type *ty) {
	if(ty->isArrayTy()) {
		ArrayType *arrTy = dyn_cast<ArrayType>(ty);
		return getArrayElementType(arrTy->getElementType());
	}
	return ty;
}

/**                                                                                                                                     
 * Dump the various different types of Array
 */

void DaikonPass::dumpArrays(fstream &declFile,
		Value *arrayElement, Type *ty,int tabCount,bool isGlobalArray) {	
	if(declFile.is_open()) {
		string    elementName = arrayElement->getName().trim().str();
		//Handle the array pointer iteself
		putTabInFile(declFile,tabCount);
		if(isGlobalArray) {
			declFile<<"variable ::"<<elementName<<"\n";
		}else {
			declFile<<"variable "<<elementName<<"\n";
		}

		SequentialType *ptrType = dyn_cast<SequentialType>(ty);
		//string typeString = getTypeString(ptrType->getElementType());
		string typeString = getArrayElementTypeString(ty);

		tabCount = 2;
		putTabInFile(declFile,tabCount);
		declFile<<"var-kind variable \n";
		if(typeString == CHAR_TYPE) {
			putTabInFile(declFile,tabCount);
			declFile<<"reference-type offset\n";

			putTabInFile(declFile,tabCount);
			declFile<<"rep-type string\n";

			putTabInFile(declFile,tabCount);
			declFile<<"dec-type char*\n";

		} else if (typeString == STRUCT_TYPE) { //Handle the structure type
		
		}else {
			putTabInFile(declFile,tabCount);                      
			declFile<<"rep-type hashcode\n";

			putTabInFile(declFile,tabCount);		
			declFile<<"dec-type "<<typeString<<"*\n";
			putTabInFile(declFile,tabCount);
			declFile<<"flags non_null\n";

			tabCount = 1;
			putTabInFile(declFile,tabCount);
			if(isGlobalArray) {
				declFile<<"variable ::"<<elementName<<"[..]\n";
			}else {
				declFile<<"variable "<<elementName<<"[..]\n";
			}

			tabCount = 2;
			putTabInFile(declFile,tabCount);
			declFile<<"var-kind "<<getTypeString(ty)<<"\n";
			putTabInFile(declFile,tabCount);
			if(isGlobalArray) {
				declFile<<"enclosing-var ::"<<elementName<<"\n";;
			} else {
				declFile<<"enclosing-var "<<elementName<<"\n";;
			}
			putTabInFile(declFile,tabCount);
			declFile<<"reference-type offset\n";
			putTabInFile(declFile,tabCount);
			declFile<<"array 1\n";

			putTabInFile(declFile,tabCount);
			declFile<<"rep-type "<<getRepTypeString(getArrayElementType(ty))<<"[]\n";
			putTabInFile(declFile,tabCount);
			declFile<<"dec-type "<<typeString<<"[]\n";
			putTabInFile(declFile,tabCount);
			declFile<<"flags non_null\n";
		}
	}
}

/**
 * This Function will create declfile
 * */

void DaikonPass::dumpDeclFileAtEntryAndExit(Function *func,string EntryOrExit, fstream &declFile) {
	int tabCount  = 0 ;
	if(declFile.is_open()) {
		StringRef funcName = func->getName().trim();// (func->getName().trim().data());
		if(find(programPoints.begin(),programPoints.end(),funcName) != programPoints.end()) {
			declFile<<"ppt .."<<funcName.data()<<"(";
			//Just Now I discovered that for c  language Daikon is not sending the arguments
			//So commnecting this code for time being
			//for(Function::arg_iterator argItr = func->arg_begin(); argItr != func->arg_end(); ++argItr) {
			//	if(argItr != func->arg_begin()) {
			//		declFile<<",";
			//	}
			//	declFile<<getTypeString(&*argItr);
			//}
			
			if(EntryOrExit == "ENTRY") {
				declFile<<"):::ENTER\n";
				tabCount = 1;
				putTabInFile(declFile,tabCount);
				declFile<<"ppt-type enter\n";
			}else if( EntryOrExit == "EXIT") {

				declFile<<"):::EXIT0\n";
				tabCount = 1;
				putTabInFile(declFile,tabCount);
				declFile<<"ppt-type subexit\n";
			}
			errs()<<"Size of the global variable list" <<globalList.size()<<"\n";
			//Process the Global values
			for(vector<Value*>::iterator globalItr = globalList.begin(); globalItr != globalList.end(); ++globalItr) {				
				GlobalVariable *v = static_cast<GlobalVariable*>(*globalItr);
				Value *globalValue = v->getInitializer();
				if( globalValue  && !isSupportedType(globalValue)) {
					continue;
				}
				//string repTypeString = getRepTypeString(v->getInitializer());
				Type *ty = getGlobalType(v->getType());
				string repTypeString = getRepTypeString(ty);
				//Nested structure members should be treated differently
				if(repTypeString == STRUCT_TYPE ) {
					dumpStructureMembers(declFile,v,ty,1,true);
				
				}else if (repTypeString == ARRAY_TYPE){
					dumpArrays(declFile,v,ty,1,true);
				}else {
					string varName = v->getName().trim().str();
					tabCount = 1;
					putTabInFile(declFile,tabCount);
					declFile<<"variable ::"<<varName<<"\n";
					tabCount =2;
					putTabInFile(declFile,tabCount);
					declFile<<"var-kind variable\n";
					putTabInFile(declFile,tabCount);
					declFile<<"rep-type "<<repTypeString<<"\n";;
					putTabInFile(declFile,tabCount);
					declFile<<"dec-type "<<getDecTypeString(ty)<<"\n";
				}
			
			}
			//Process function Params
			for(Function::arg_iterator argItr = func->arg_begin(); argItr != func->arg_end(); ++argItr) {
				Argument *arg = &*argItr;
				Value *v = static_cast<Value*>(arg);
				if(!isSupportedType(v)) continue;
				string varName = v->getName().trim().str();
				string typeString = getTypeString(v);
				StringRef typeStringRef(typeString);
				errs()<<"Type string for the argument is : "<<typeStringRef<<"\n";
				if(typeString == POINTER_TYPE) {
					PointerType *ptrType = dyn_cast<PointerType>(v->getType());
					if(arg->hasByValAttr() && 
							getTypeString(ptrType->getContainedType(0)) == STRUCT_TYPE) {
						dumpStructureMembers(declFile,v,ptrType->getContainedType(0),1,false);
					}
				}else {
					tabCount = 1;
					putTabInFile(declFile,tabCount);
					declFile<<"variable "<<varName<<"\n";
					tabCount = 2;
					putTabInFile(declFile,tabCount);
					declFile<<"var-kind variable\n";
					putTabInFile(declFile,tabCount);
					declFile<<"rep-type "<<getRepTypeString(v)<<"\n";
					putTabInFile(declFile,tabCount);
					declFile<<"dec-type "<<getDecTypeString(v)<<"\n";
					putTabInFile(declFile,tabCount);
					declFile<<"flags is_param\n";
				}
			}

			if(EntryOrExit == "EXIT") {
			    string returnType = getTypeString(func->getReturnType());
			   // if(returnType == "int") {
			   if(isSupportedType(func->getReturnType())) {

				tabCount = 1;
				putTabInFile(declFile,tabCount);
				declFile<<"variable return\n";
				tabCount = 2;
				putTabInFile(declFile,tabCount);
				declFile<<"var-kind variable\n";
				putTabInFile(declFile,tabCount);
				string repTypeString = getRepTypeString(func->getReturnType());
				declFile<<"rep-type "<<repTypeString<<"\n";
				putTabInFile(declFile,tabCount);
				declFile<<"dec-type "<<returnType<<"\n";
			    }
			}

		}
	}
}

void DaikonPass::dumpForHookAfterFunction(fstream &declFile, string EntryOrExit,Function *func) {
	
	int tabCount  = 0 ;
	if(declFile.is_open()) {
		declFile<<"ppt ..hookAfter(";

		if(EntryOrExit == "ENTRY") {
			declFile<<"):::ENTER\n";
			tabCount = 1;
			putTabInFile(declFile,tabCount);
			declFile<<"ppt-type enter\n";
		}else if( EntryOrExit == "EXIT") {

			declFile<<"):::EXIT0\n";
			tabCount = 1;
			putTabInFile(declFile,tabCount);
			declFile<<"ppt-type subexit\n";
		}

		errs()<<"Size of the argument list : "<<func->getArgumentList().size()<<"\n";

		//Process function Params
		for(Function::arg_iterator argItr = func->arg_begin(); argItr != func->arg_end(); ++argItr) {
			Argument *arg = &*argItr;
			Value *v = static_cast<Value*>(arg);
			string varName = v->getName().trim().str();
			errs()<<"Name of the variable :"<<v->getName()<<":\n";
			/**
			 * We are not reporting anything other than int.
			 * So doing this filtering
			 */
			tabCount = 1;
			putTabInFile(declFile,tabCount);
			//declFile<<"variable "<<varName<<"\n";
			declFile<<"variable val\n";
			tabCount = 2;
			putTabInFile(declFile,tabCount);
			declFile<<"var-kind variable\n";
			putTabInFile(declFile,tabCount);
			declFile<<"rep-type "<<"int"<<"\n";
			putTabInFile(declFile,tabCount);
			declFile<<"dec-type "<<"int"<<"\n";
			putTabInFile(declFile,tabCount);
			declFile<<"flags is_param\n";
			declFile<<"\n";
		}


	}
}


void DaikonPass::dumpDeclFile(Module &module) {
	fstream declFile("program.dtrace",ios::out);
	if(declFile.is_open()) {
		declFile<<"input-language C/C++\n";
		declFile<<"decl-version 2.0\n";
		declFile<<"var-comparability none\n\n";

		//This is for hookAfter function 
		//They are not active anymore
		//dumpForHookAfterFunction(declFile,"ENTRY");
		//dumpForHookAfterFunction(declFile,"EXIT");

		for (Module::iterator funcItr = module.begin(); funcItr != module.end(); ++funcItr) {
			Function *func = &(*funcItr);
			string funcName = func->getName().trim().str();

			if(find(programPoints.begin(),programPoints.end(),funcName) != programPoints.end()) {
				dumpDeclFileAtEntryAndExit(func,"ENTRY",declFile);
				declFile<<"\n";
				dumpDeclFileAtEntryAndExit(func,"EXIT",declFile);
				declFile<<"\n";
			}else if(funcName == "hookAfter") {
				dumpForHookAfterFunction(declFile,"ENTRY",func);
				dumpForHookAfterFunction(declFile,"EXIT",func);
			}
		}
		declFile.close();
	}
}


/**
 * This Function inserts instructions at global variables access which will fake a function call in Daikon perspective.
 * Using this we are able generate invariants at global variables access
 */
void DaikonPass::insertDynamicCallAtGlobalAccess(Function *func) {
	Module *module = func->getParent();
	if(!isInit) {
		doInit(module);
	}

        //The space is not sent then it will be initialized to -1
	//In that case it will follow the old schedule
	if(space == -1 ) {

		for(inst_iterator instItr = inst_begin(func); instItr != inst_end(func); ++instItr) { 
			Instruction *instr = NULL;
			Value *operand = NULL;
			if(StoreInst *storeInst = dyn_cast<StoreInst>(&*instItr)) {
				operand = storeInst->getPointerOperand();
			}else if(LoadInst *loadInst = dyn_cast<LoadInst>(&*instItr)) {
				operand = loadInst->getPointerOperand();
			}
			if(operand != NULL && operand->hasName() && operand->getName().equals("__clapDummyGlobalVar" )) {
				continue;		
			}
			instr = dyn_cast<Instruction>(&*instItr);

			//	if(StoreInst *storeInst = dyn_cast<StoreInst>(&*instItr)) 
			if(operand != NULL) {
				//errs()<<"Operand  name : " <<operand->getName()<<"\n";
				// if(isGlobal(operand)) 
				//if(GlobalVariable *gVal = dyn_cast<GlobalVariable>(operand))

				if(isGlobal(operand)) {
					GlobalVariable *gVal = dyn_cast<GlobalVariable>(operand);
					string functionName= "";			
					string typeString = getTypeString(gVal->getInitializer()->getType());

					//if(typeString == "int") {
						MDNode *metaNode = instr->getMetadata("dbg");
						if(metaNode != NULL) {
							string fun1 = declDumperForGlobals(operand,true,instr);
							string fun2 = declDumperForGlobals(operand,false,instr);
							assert(fun1 == fun2);
							functionName = fun1;
						}else {
							string fun1 = declDumperForGlobals(operand,true);
							string fun2 = declDumperForGlobals(operand,false);
							assert(fun1 == fun2);
							functionName = fun1;
						}
				       // }else {
					//	errs()<<"We are not handling other types currently\n";
				       // }

					// Now insert code to
					Function *hookChc =  cast<Function>(module->getOrInsertFunction("clap_chcHook",functionType));
					vector<Value*> entryArgList;
					vector<Value*> exitArgList;
					//Function arguments will be 
					//0. variable count
					//1. Entry/Exit an int variable : 1 for entry and 0 for exit
					//2. variable name  : char *
					//3. variable type  : char *
					//4. variable value : int* (since this is global variable)
					//5. Finally Send the Function Name 
					//So total argCount (execpt argcount variable is 5
					const int ARGUMENT_COUNT = 5; // See above
					Constant *argCount = ConstantInt::get(module->getContext(), APInt(32,ARGUMENT_COUNT,true));
					entryArgList.push_back(argCount);
					//Entry flag
					Constant *entry = ConstantInt::get(module->getContext(), APInt(32,1,true));
					entryArgList.push_back(entry);
					Value *val = operand;
					Value *valName = getValueForString (val->getName(),module);
					//Value *type = getValueForString(StringRef(getTypeString(val).c_str()).trim(),module);
					Value *type = getValueForString(StringRef("int").trim(),module);
					entryArgList.push_back(valName);
					entryArgList.push_back(type);
					entryArgList.push_back(val);
					entryArgList.push_back(getValueForString(StringRef(functionName.c_str()),module));

					Constant *exit = ConstantInt::get(module->getContext(),APInt(32,0,true));
					exitArgList.push_back(argCount);
					exitArgList.push_back(exit);
					exitArgList.push_back(valName);
					exitArgList.push_back(type);
					exitArgList.push_back(val);
					exitArgList.push_back(getValueForString(StringRef(functionName.c_str()),module));

					Instruction *hookChcBeforeInstr = CallInst::Create(hookChc,entryArgList);				
					hookChcBeforeInstr->insertBefore(instr);

					Instruction *hookChcAfterInstr = CallInst::Create(hookChc,exitArgList);
					hookChcAfterInstr->insertAfter(instr);
				}
			}

		}

	} else {
		//If the space is 0 then it will record every global read and write
		//1 means there will be gap of 1 instruction
		errs()<<"Value of the spacer is "<<space<<"\n";
		errs()<<"Modifying for the function "<<func->getName()<<"\n";
		vector<Instruction*> loadAndStores;

		for(inst_iterator instItr = inst_begin(func); instItr != inst_end(func); ++instItr) { 
			if(isa<StoreInst>(&*instItr) || isa<LoadInst>(&*instItr)) {
				Instruction *ii = &*instItr;
				Value *ptrOp =NULL;
				if(isa<StoreInst>(ii)) {
					StoreInst *st_ii = dyn_cast<StoreInst>(ii);
					ptrOp = st_ii->getPointerOperand();
				}else if(isa<LoadInst>(ii)){
					LoadInst *ld_ii = dyn_cast<LoadInst>(ii);
					ptrOp = ld_ii->getPointerOperand();
				}
				if(ptrOp == NULL || !isGlobal(ptrOp)) {
					continue;
				}
				loadAndStores.push_back(ii);
			}
		}
		errs()<<"Load and Store size : "<<loadAndStores.size()<<"\n";
		if(loadAndStores.size() > 0) {
			for(vector<Instruction*>::iterator lstItr = loadAndStores.begin(); lstItr != loadAndStores.end(); ++lstItr) { 
				vector<Instruction*> internal;
			       // set<string> globVarSet;
			       	vector<string> globVarSet;
				int count = 0 ;
				for(vector<Instruction*>::iterator itr = lstItr; count <= space && itr != loadAndStores.end()
						; ++count,++itr) {
					Instruction *ii =  *itr;
					Value *ptrOp =NULL;
					if(isa<StoreInst>(ii)) {
						StoreInst *st_ii = dyn_cast<StoreInst>(ii);
						ptrOp = st_ii->getPointerOperand();
					}else if(isa<LoadInst>(ii)){
						LoadInst *ld_ii = dyn_cast<LoadInst>(ii);
						ptrOp = ld_ii->getPointerOperand();
					}
					//Now this will be global
					GlobalVariable *glVar = dyn_cast<GlobalVariable>(ptrOp);
					assert(glVar != NULL);
				
					string tt = getTypeString(glVar->getInitializer());
				       // StringRef typeStr(getTypeString(glVar->getInitializer()));
				       // errs()<<"\n--------------\n";
				       // //errs()<<"Type is : " <<typeStr<<" for "<<*ptrOp<<"\n";
				       // errs()<<"Is global : "<<isGlobal(ptrOp)<<"  "<<*ptrOp<<"\n";
				       // errs()<<"Name : "<<ptrOp->getName()<<"\n";
				       // errs()<<"Condition "<<(ptrOp != NULL && ptrOp->hasName() && 
				       // 		!(ptrOp->getName().equals("__clapDummyGlobalVar")) && isGlobal(ptrOp))<<"\n";// && typeStr.equals("int"))<<"\n";
				       // //errs()<<"Type condition :"<<typeStr.trim().equals("int")<<" for type : "<<typeStr<<"\n";
				       // errs()<<"\n--------------\n";

					if(ptrOp != NULL && ptrOp->hasName() && 
							!(ptrOp->getName().equals("__clapDummyGlobalVar"))
							&& isGlobal(ptrOp)) {
							//&& tt == "int") {
						internal.push_back(ii);				
						errs()<<"Inserting : "<<*ptrOp<<"\n";
						if(find(globVarSet.begin(),globVarSet.end(),
									ptrOp->getName().trim().str()) == globVarSet.end()) {

							globVarSet.push_back(ptrOp->getName().trim().str());
						}
						//globVarSet.insert(ptrOp->getName().trim().str());
					}
				}

				//errs()<<"Size of the internal "<<internal.size()<<"\n";
				//errs()<<"Showing internal ---- \n";
                                //for(vector<Instruction*>::iterator ai = internal.begin(); ai != internal.end(); ++ai) {
				//	Instruction *t = *ai;
				//	errs()<<"Instruction is : "<<*t<<"\n";
				//}

				if(internal.size() > 0){
					//errs()<<"First Instruction " <<*(internal.front())<<"\n";
					//errs()<<"Last Instruction  "<<*(internal.back())<<"\n";
					string funcName1  = declDumperForCSGlobals(globVarSet,internal.front(),internal.back(),true);
					string funcName2  = declDumperForCSGlobals(globVarSet,internal.front(),internal.back(),false);
					assert(funcName1 == funcName2);
					string functionName = funcName1;
					
					//We have to generate the op set as some operand may be repeated with in the 
					//expected CS
					vector<Value*> operandSet;
					for(vector<Instruction *>::iterator opCreateItr = internal.begin(); opCreateItr != internal.end();
							++opCreateItr) {
						Value *pointer = NULL;
						if(StoreInst *st = dyn_cast<StoreInst>(*opCreateItr)) {
							pointer = st->getPointerOperand();
						}else if(LoadInst *ld = dyn_cast<LoadInst>(*opCreateItr)) {
							pointer = ld->getPointerOperand();
						}else {
							errs()<<"Impossible case\n";
						}
						//errs()<<"Name of the pointer op "<<pointer->getName()<<"\n";
						string opName = pointer->getName().trim().str();
						bool flag = false;
						for(vector<Value*>::iterator opSetItr = operandSet.begin(); opSetItr != operandSet.end(); ++opSetItr) {
							Value *v = *opSetItr;
							if(v->hasName()) {
								StringRef s = v->getName();
								if(s.equals(pointer->getName())) {
									flag = true;
									break;

								}
							}
						}
						if(!flag) {
							operandSet.push_back(pointer);
						}

					}
					assert(operandSet.size() == globVarSet.size());
					
				       // // Now insert code to
				        Function *hookChc =  cast<Function>(module->getOrInsertFunction("clap_chcHookDynamic",functionType));
				        vector<Value*> entryArgList;
				        vector<Value*> exitArgList;
				        //Function arguments will be 
				        //0. variable count
				        //1. Entry/Exit an int variable : 1 for entry and 0 for exit
				        //2.1. Number of vars : int
				        //5. Finally Send the Function Name 
				        //2. variable name  : char *
				        //3. variable type  : char *
				        //4. variable value : int* (since this is global variable)
				        //-. 2 -3 -4 will be repeated as the # of vars 
				        //So total variable count will be  1+(2-3-4)* variable # +5
				        const int ARGUMENT_COUNT = 2+globVarSet.size(); // See above
				        Constant *argCount = ConstantInt::get(module->getContext(), APInt(32,ARGUMENT_COUNT,true));
				        entryArgList.push_back(argCount);
				        //Entry flag
				        Constant *entry = ConstantInt::get(module->getContext(), APInt(32,1,true));
				        entryArgList.push_back(entry);
					Constant *numOfVars = ConstantInt::get(module->getContext(),APInt(32,operandSet.size(),true));
					entryArgList.push_back(numOfVars);
					entryArgList.push_back(getValueForString(StringRef(functionName.c_str()),module));
				
					//Now iterate it for each vars
					for(Value *v : operandSet) {
						Value *val = v;
						Value *valName = getValueForString (val->getName(),module);
						Value *type = getValueForString(StringRef(getTypeString(val).c_str()).trim(),module);
						entryArgList.push_back(valName);
						entryArgList.push_back(type);
						entryArgList.push_back(val);
					}


					Constant *exit = ConstantInt::get(module->getContext(),APInt(32,0,true));
				        exitArgList.push_back(argCount);
				        exitArgList.push_back(exit);
					exitArgList.push_back(numOfVars);
					exitArgList.push_back(getValueForString(StringRef(functionName.c_str()),module));
					
					for(Value *v : operandSet) {
						Value *val = v;
						Value *valName = getValueForString (val->getName(),module);
						Value *type = getValueForString(StringRef(getTypeString(val).c_str()).trim(),module);
						exitArgList.push_back(valName);
						exitArgList.push_back(type);
						exitArgList.push_back(val);
					}
					

					Instruction *hookChcBeforeInstr = CallInst::Create(hookChc,entryArgList);				
				        hookChcBeforeInstr->insertBefore(internal.front());

				        Instruction *hookChcAfterInstr = CallInst::Create(hookChc,exitArgList);
				        hookChcAfterInstr->insertAfter(internal.back());
				}

				errs()<<"\n********************\n\n";
			}
		}
	}
}


string DaikonPass::declDumperForCSGlobals(vector<string> variableNames, 
		Instruction *firstInstruction,Instruction *lastInstruction, bool EntryOrExit) {
        string functionName = "";
	StringRef ff = "";
	bool debugInfoPresent = false;
	MDNode *metadata = firstInstruction->getMetadata("dbg");
	if(metadata != NULL) {
		debugInfoPresent = true;
		DILocation Loc(metadata);
		if (!Loc.Verify()) {
			ff = "";
		}else {
			ff = Loc.getFilename();
		}
	}
	if(debugInfoPresent) {
		//Function name to be generated in this case will be in the format of 
		//<source_file_name>+<_>+<line # of the start Instrucion>+<_>+
		//<line # of the last Instruction+<_>+<coutner_value>
		StringRef fileNameRef  = ff;//getDebugFileName(firstInstruction);
		string startLineNumber = to_string(getDebugLineNum(firstInstruction));
		string endLineNumber   = to_string(getDebugLineNum(lastInstruction));
		functionName 	       = fileNameRef.str()+"_"+startLineNumber+"_"+endLineNumber+"_"+to_string(counter);

		//Increase the counter in case of exit
		if(!EntryOrExit) {
			counter++;
		}
		fstream declFile("program.dtrace", ios::out|ios::app);
		int tabcount = 0 ; 
		if(declFile.is_open()) {
			if(EntryOrExit){ //True for entry type
				declFile<<"ppt .."<<functionName<<"():::ENTER\n";
				tabcount = 1;
				putTabInFile(declFile,tabcount);
				declFile<<"ppt-type enter\n";

			}else {
				declFile<<"ppt .."<<functionName<<"():::EXIT0\n";
				tabcount = 1;
				putTabInFile(declFile,tabcount);
				declFile<<"ppt-type subexit\n";
			}

                        //Due to the global variable Reading writing issue
			//We need the reverse the set here
			
			for(string var:variableNames) {
				putTabInFile(declFile,tabcount);
				declFile<<"variable ::"<<var<<"\n";
				tabcount=2;
				putTabInFile(declFile,tabcount);
				declFile<<"var-kind variable\n";
				putTabInFile(declFile,tabcount);
				declFile<<"rep-type int\n";
				putTabInFile(declFile,tabcount);
				declFile<<"dec-type int\n";
                                tabcount = 1;
			}
			declFile<<"\n";
			declFile.close();

		}
	}
	return functionName;

}
 
/**
 * This function will dump the decl information . This information
 * will be used by daikon for fake function call( dynami functions)
 * at the global access.
 * */
string DaikonPass::declDumperForGlobals(Value *value ,bool EntryOrExit,Instruction *instr) {
	string variableName = value->getName().trim().str();
	string functionName= ""; 
	if(instr!= NULL) {
		StringRef fileName = getDebugFilename(instr);
		string lineNumber = to_string(getDebugLineNum(instr));
		functionName = fileName.trim().str()+"_"+lineNumber+"_"+to_string(counter);
	}else {
		"Arijit"+to_string(counter);
	}
	//if(!EntryOrExit && instr == NULL) {
	if(!EntryOrExit) {
		++counter;
	}
	fstream declFile("program.dtrace",ios::out|ios::app);
	int tabcount = 0 ;
	if(declFile.is_open()) {
		if(EntryOrExit){ //True for entry type
			declFile<<"ppt .."<<functionName<<"():::ENTER\n";
			tabcount = 1;
			putTabInFile(declFile,tabcount);
			declFile<<"ppt-type enter\n";

		}else {
			declFile<<"ppt .."<<functionName<<"():::EXIT0\n";
			tabcount = 1;
			putTabInFile(declFile,tabcount);
			declFile<<"ppt-type subexit\n";
		}
		putTabInFile(declFile,tabcount);
		declFile<<"variable ::"<<variableName<<"\n";
		tabcount=2;
		putTabInFile(declFile,tabcount);
		declFile<<"var-kind variable\n";
		putTabInFile(declFile,tabcount);
		declFile<<"rep-type int\n";
		putTabInFile(declFile,tabcount);
		declFile<<"dec-type int\n";
		declFile<<"\n";

		declFile.close();
	}
	return functionName;
}





void DaikonPass::displayGlobalVars() {
	for(Value *v :globalList) {
		errs()<<"Global Variables are "<<*v<<"\n";
	}
}

/**
 * DummyVarInsertionPass Functions
 */

DummyVarInsertionPass::DummyVarInsertionPass():ModulePass(ID) {

}

bool DummyVarInsertionPass::runOnModule(Module &module) {
	//StringRef dummyVar("DUMMY_INIT");
	//Constant *valueName = ConstantDataArray::getString(module.getContext(), dummyVar,true);
	Constant *valueName = ConstantInt::get(module.getContext(), APInt(32,1,true));
	dummyVar = new GlobalVariable(module,valueName->getType(), false, GlobalValue::InternalLinkage,valueName);
	Value *val = dyn_cast<Value>(dummyVar);
	val->setName("__clapDummyGlobalVar");
	
	for(Module::iterator funcItr = module.begin(); funcItr != module.end(); ++funcItr) {
		Function *func = &*funcItr;
		if(func->size() != 0) {
			insertDummyStoreIntoFunction(func);
		}
	}

	return true;
}

void DummyVarInsertionPass::insertDummyStoreIntoFunction (Function *func) {
	Instruction *target = NULL;
	for(inst_iterator instItr = inst_begin(func) ; instItr != inst_end(func) ; ++instItr) {
		if(!isa<AllocaInst>(&*instItr)) {
			target = &*instItr;
			break;
		}
	}
	if(target != NULL) {
		StoreInst *storeInst = new StoreInst(dummyVar->getInitializer(),dummyVar,0,target);
	}
}




/**
 * Concurrent begin
 */


/**
 * All the test application methods will be here..
 */
void DaikonPass::testMethod(Function *func) {
	for(inst_iterator instItr = inst_begin(func); instItr != inst_end(func); ++instItr) {
		Instruction *inst = &*instItr;
		if(StoreInst *storeInst = dyn_cast<StoreInst>(inst)) {
			handleStoreInst(storeInst);
		}
	}

	for(inst_iterator instItr = inst_begin(func); instItr != inst_end(func); ++instItr) {
		Instruction *inst = &*instItr;
		if(StoreInst *storeInst = dyn_cast<StoreInst>(inst)) {
			blockBreaker(storeInst);
			break;
		}
	}
	
}

void DaikonPass::blockBreaker(Instruction *inst) {
	BasicBlock *parentBlock = inst->getParent();
	BasicBlock *newBlock = SplitBlock(inst->getParent(),inst,this);	
}
                                               
void DaikonPass::handleStoreInst(StoreInst *storeInst) {
	//errs()<<"Store instruction received " <<*storeInst<<"\n";
	if(storeInst->getPointerOperand() == NULL || !storeInst->getPointerOperand()->hasName()) {
		errs()<<"Returning for wrong store instruction " <<*storeInst<<"\n";
		return ;
	}
	StringRef opNameRef = storeInst->getPointerOperand()->getName();
	Value *valueOp = storeInst->getValueOperand();

	//errs()<<"\n----- Start -----\n";
        //errs()<<"Store instruction " <<*storeInst<<"\n";
	//errs()<<"Value is  : "<<*valueOp<<"\n";
	//errs()<<"\n----- END -----\n";
	
	switch(valueOp->getType()->getTypeID()) {
		case 10: {
			 
				 if(ConstantInt *intVal = dyn_cast<ConstantInt>(valueOp)) {
					 int64_t data = intVal->getValue().getSExtValue();
					 StringRef dataString(to_string(data));
					 errs()<<opNameRef<<" = "<<dataString<<"\n";
				 }else {
					if(LoadInst *loadInst = dyn_cast<LoadInst>(valueOp)) {
						if(loadInst->getPointerOperand() && loadInst->getPointerOperand()->hasName()) {
							errs()<<opNameRef<<" = "<<loadInst->getPointerOperand()->getName()<<"\n";
						}

					}else if(AllocaInst *allocaInst = dyn_cast<AllocaInst>(valueOp)) {
						if(allocaInst->hasName()) {
							errs()<<opNameRef<<" = "<<allocaInst->getName()<<"\n";
						}
					}else if(BinaryOperator *binOp = dyn_cast<BinaryOperator>(valueOp)){
						Instruction::BinaryOps opCode = binOp->getOpcode();
						StringRef symbol;
						switch(opCode) {
							case Instruction::Add:
							case Instruction::FAdd:
								symbol = " + ";
								break;

							case Instruction::Sub:
							case Instruction::FSub:
								symbol = " - ";
								break;

							case Instruction::Mul:
							case Instruction::FMul:
								symbol  = " * ";
								break;

							case Instruction::UDiv:
							case Instruction::SDiv:
							case Instruction::FDiv:
								symbol = " / ";
								break;

							case Instruction::URem:
							case Instruction::SRem:
							case Instruction::FRem:
								symbol = " % ";
								break;

							default:
								symbol = " <Unknow> ";
						}
//						errs()<<"Symbol is  : "<<symbol<<"\n";
						StringRef firstOp;
						StringRef secondOp;
						for(unsigned int c =0 ; c < binOp->getNumOperands(); ++c) {
							StringRef opStr;
							Value *op = binOp->getOperand(c);
							if(ConstantInt *intOp = dyn_cast<ConstantInt>(op)) {
								int64_t dataOp = intOp->getValue().getSExtValue();
								opStr = to_string(dataOp);
							}else if (LoadInst *opLoadInst = dyn_cast<LoadInst>(op)){
								opStr = opLoadInst->getPointerOperand()->getName();
							}else {
								errs()<<"Op is "<<*op<<"\n";
							}
							switch(c) {
								case 0 :
									firstOp = opStr;
									break;

								case 1:
									secondOp = opStr;
									break;

								default:
									errs()<<"Unhandled case\n";
							}
						}
						//CmpInst *cmp = CmpInst::Create(Instruction::ICmp,ICMP_EQ,storeInst->getPointerOperand(),
						//errs()<<opNameRef<<" = "<<*(binOp->getOperand(0))<<symbol<<*(binOp->getOperand(1))<<"\n";
						errs()<<opNameRef<<" = "<<firstOp<<symbol<<secondOp<<"\n";
					}
				 
				 }
				 break;
			 }

		case 2:{
		       	errs()<<"Float type is yet to handle\n";
			break;
		       }

		default:
			 errs()<<"Failed to act for the instruction "<<*storeInst<<"  type  : "<<valueOp->getType()->getTypeID()<<"\n";
	}
}


/**
 * Concurrent ends
 */

char DaikonPass::ID = 0;
bool DaikonPass::isInit = false;
RegisterPass<DaikonPass> X("form", "This pass is to interface Daikon with LLVm", false,false);


char DummyVarInsertionPass::ID = 0;
RegisterPass<DummyVarInsertionPass> dummyPass("dummy","This pass will insert the dummy global variable access",false,false);
