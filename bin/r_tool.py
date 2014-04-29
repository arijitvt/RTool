#!/usr/bin/env python



import os;
import sys;
import shutil;

#Generic utility to build the command string
blank = " ";

#inspect related path setting
inspect_path 	 = "/home/ari/cvs/src/inspect-0.3";
inspect_lib_path = inspect_path+"/lib";
ld_lib_flag 	 = " -Wl,-rpath,"+inspect_lib_path;
ld_lib_flag 	 += " -Wl,-rpath-link,"+inspect_lib_path;
ld_lib_flag 	 += " -Wl,-L,"+inspect_lib_path;

#llvm Related path setting
llvm_src_path 	= "" ;
llvm_build_path = "/home/ari/cvs/src/build/Release+Debug+Asserts";
llvm_lib_path 	= llvm_build_path+"/lib";
llvm_bin_path 	= llvm_build_path+"/bin" ;


#Again update the lib path
ld_lib_flag 	 += " -Wl,-rpath,"+llvm_lib_path;
ld_lib_flag 	 += " -Wl,-rpath-link,"+llvm_lib_path;
ld_lib_flag 	 += " -Wl,-L,"+llvm_lib_path;


#print "Lib Path "+ld_lib_flag;

#Custom commands for llvm tools
clang_tool   	= llvm_bin_path+"/clang" ;
opt_tool     	= llvm_bin_path+"/opt"   ;
llvm_link_tool 	= llvm_bin_path+"/llvm-link";
llvm_dis_tool 	= llvm_bin_path+"/llvm-dis" ;
llvm_llc_tool 	= llvm_bin_path+"/llc";

#gcc compiler
gcc_tool 	= "gcc";


#link flags 
link_flags = " -linspect -lpthread -lrt -lm -lstdc++ ";

#add all compile time flags here
clang_generic_flag = "-S -emit-llvm -g";   

clap_opt_flag  = " -basicaa -tap_inst_number -tap_pthreads -tap_mem"
hook_file_path = "/home/ari/cvs/src/llvm-3.2.src/lib/Transforms/Daikon/hook.c"
daikon_src_path = "/home/ari/cvs/src/llvm-3.2.src/lib/Transforms/Daikon/"





def run_clap_pass(filename,filename_without_extension) :
	print "Running clap pass on "+filename;
	input_filename = filename;
	output_filename = filename_without_extension+".3.o"
	command_string = opt_tool+blank+" -instnamer -o "+output_filename+blank+filename;
	os.system(command_string)
	input_filename = output_filename;

	output_filename =  filename_without_extension+".4.o"
	command_string = opt_tool+blank+" -load "+llvm_lib_path+"/LLVMTAP.so "+\
			clap_opt_flag+" -o "+output_filename+blank\
				+input_filename;
	os.system(command_string)

	os.system("rm -rf "+input_filename); 
	return output_filename;


def run_daikon_pass(filename,filename_without_extension):
	print "Running Original Daikon Pass on "+filename;
	output_filename = filename_without_extension+".5.o";
	command_string = opt_tool+blank+"-load"+blank+llvm_lib_path+\
			"/DaikonPass.so -form -dumpppt=false -loadppt=true -o "+\
				output_filename+blank+filename;
	os.system(command_string);
	return output_filename

def run_daikon_pass_with_spacer(filename,filename_without_extension,spacer):
	print "Running Original Daikon Pass with spacer on "+filename+" with spacer "+str(spacer) ;
	output_filename = filename_without_extension+".5.o";
	command_string = opt_tool+blank+"-load"+blank+llvm_lib_path+\
			"/DaikonPass.so -form -dumpppt=false -loadppt=true -spacer="+str(spacer)+" -o "+\
				output_filename+blank+filename;
	os.system(command_string);
	return output_filename

def run_dummy_daikon_pass(filename,filename_without_extension):
	print "Running Dummy Daikonpass on "+filename;
	output_filename= filename_without_extension+".2.o" ;
	command_string = opt_tool+blank+"-load"+blank+llvm_lib_path+"/DaikonPass.so -dummy -o "+\
				output_filename+blank+filename;
	os.system(command_string)
	return output_filename;


def compile_by_clang(filename,filename_without_extension) :
	print "Compiling "+filename+" with clang" ;
	output_filename = filename_without_extension+".1.o";
	command_string = clang_tool+"  "+clang_generic_flag+"  -o "+\
				output_filename+" "+filename;
	os.system(command_string);
	return output_filename;

def do_final_llvm_linking(instrumented_obj_file,hook_obj_file) :
	output_filename = "final_instrumented.o"
	command_string = llvm_link_tool+blank+instrumented_obj_file+\
				blank+hook_obj_file+blank+\
					" -o "+output_filename
	os.system(command_string);
	return  output_filename;

def decompile_to_asm(filename):
	output_filename = "final.s"
	command_string = llvm_llc_tool+blank+" -o "+\
				blank+output_filename+blank+filename;
	os.system(command_string);
	return output_filename;

def gcc_compilation(filename):
	output_filename = "a.out"
	command_string  = gcc_tool+blank+" -L"+inspect_lib_path+blank+\
				" -o "+output_filename+blank+\
					filename+blank+ld_lib_flag+link_flags;
	os.system(command_string);
	return output_filename;


def compile_all_files(src,sp):
	temp_file_list = [];
	output_file = ""
	for file in os.listdir(src):
		if file.endswith(".c") and not file.startswith("hook"):
			filename_without_extension =  file[:file.rfind(".")]
			output_file = compile_by_clang(file,filename_without_extension);
			temp_file_list.append(output_file);
		        output_file = run_dummy_daikon_pass(output_file,filename_without_extension)
		        temp_file_list.append(output_file);
		        output_file = run_clap_pass(output_file,filename_without_extension)
		        temp_file_list.append(output_file);

			if sp > -1 :
				output_file =run_daikon_pass_with_spacer(output_file,filename_without_extension,sp);
			else:
				output_file = run_daikon_pass(output_file,filename_without_extension);

		        temp_file_list.append(output_file);

        shutil.copy(daikon_src_path+"hook.h",".");
        shutil.copy(daikon_src_path+"hook.c",".");
        output_file = compile_by_clang("hook.c","hook")
        output_file = do_final_llvm_linking(" *.5.o ",output_file);
	temp_file_list.append(output_file);
	output_file = decompile_to_asm(output_file);
	temp_file_list.append(output_file);
	output_file =gcc_compilation(output_file);

	return temp_file_list;

def cleanup(temp_file_list) :
	for file in temp_file_list:
		print "Deleting "+file
		os.system("rm -rf "+file);


def gen_program_point_for_file(filename):
	output_file=filename+".o"
	command_string = clang_tool+blank+clang_generic_flag+" -o "+output_file+\
				blank+filename;
	os.system(command_string);
	
	output_file_1 = filename+".1.o";	
	command_string = opt_tool+blank+"-load"+blank+llvm_lib_path+\
			"/DaikonPass.so -form -dumpppt=true -loadppt=false -o "+\
				output_file_1+blank+output_file;
	os.system(command_string);
	os.system("rm -rf "+output_file);
	os.system("rm -rf "+output_file_1);
	return ;

def dump_program_point(src):
	os.system("rm -rf ProgramPoints.ppts");
	os.system("touch ProgramPoints.ppts");
	for file in os.listdir(src):
		if file.endswith(".c") and not file.startswith("hook"):
			gen_program_point_for_file(file);
			
	return ;



def main_controller():
	argList = sys.argv
	if(len(argList) < 3) :
		print "Please enter the commands"
		exit(0);

	option = argList[1];
	print "Option is "+option

	SRC_PATH = argList[2]

	if option == "--dump-points" :
		print "Yes coming"
		dump_program_point(SRC_PATH);
		#do something


	elif option == "--run" :
       	 	temp_file_list = compile_all_files(SRC_PATH,-1);
		cleanup(temp_file_list);

	elif option == "--run_sp":
		sp_val = argList[3]
		print "Sp value : "+str(sp_val)
       	 	temp_file_list = compile_all_files(SRC_PATH,sp_val);
		cleanup(temp_file_list);

	return;


main_controller();





