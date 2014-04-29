#!/usr/bin/env python

import os;
import sys;
import shutil;

daikon_path="/home/ari/cvs/src/llvm-3.2.src/lib/Transforms/Daikon"

#Original Diakon Run Script Starts

def compile():
	command_string = "gcc -gdwarf-2 -o output main.c -pthread ";
	os.system(command_string);                   
	return;


def  dump_ppt_and_decl():
	command_string = "kvasir-dtrace --dump-ppt-file=prog.ppts --dump-var-file=prog.vars ./output";
	os.system(command_string);	
	return ;

def run_executable():
	flagDeclExists = False;
	flagPptExists  = False;
	#for Pfscan
	argument = " -i -n 4 arijit"

	for file in os.listdir("."):
		if ".vars" in file :
			flagDeclExists = True;
			break;

	for file in os.listdir("."):
		if ".ppts" in file :
			flagPptExists = True;
			break;

	if flagPptExists and flagPptExists : 
		command_string = "kvasir-dtrace --ppt-list-file=prog.ppts --var-list-file=prog.vars ./output "+argument;
		os.system(command_string);
	elif flagPptExists:
		
		command_string = "kvasir-dtrace --ppt-list-file=prog.ppts  ./output "+argument;
		os.system(command_string);

	elif flagDeclExists:
		command_string = "kvasir-dtrace  --var-list-file=prog.vars ./output "+argument;
		os.system(command_string);

	return;

def dtrace_manip_for_daikon(counter):
	command_string = "cp -r daikon-output/*.dtrace arijit/"
	os.system(command_string);
	os.chdir("arijit");
	for file in os.listdir("."):
		if not "_" in file: 
			file_names = file.split(".");
			assert len(file_names) == 2;
			final_name =  file_names[0]+"_"+str(counter)+"."+file_names[1];
			os.rename(file,final_name)
	os.chdir("../");
	return 
def run_daikon():
	command_string = "java daikon.Daikon daikon-output/output.dtrace"
	os.system(command_string);
	os.system0
#Orginal Daikon Run Script Ends                                         



#Create the directory structure to initializing the test cases
def init():
	if not os.path.exists("arijit"):
		os.mkdir("arijit");	

def usage_for_daikon():
	msg =  "First you run tukli.py dump to compile and dump the ppts and vars files\n  \
		 Modify the files if needed and then run tukli.py run to run for the first time\n \
		 And then tukli.py rep #, where # is the run count , must be greater or equal to 1";	

	print msg;

def main_daikon() :
	init();
	if len(sys.argv) == 1 :
		usage_for_daikon();
		return;

	elif sys.argv[1] == "help":
		usage_for_daikon();

	if sys.argv[1] == "dump":   
		#shutil.copy(daikon_path+"/hook.h", ".");
		compile();
		dump_ppt_and_decl();

	elif sys.argv[1] == "run" :
		#assert len(sys.argv) == 3;
		run_executable();
		dtrace_manip_for_daikon(0);

	elif sys.argv[1] == "rep" :
		assert len(sys.argv) == 3 and sys.argv >= 1;
		compile();
		run_executable();
		dtrace_manip_for_daikon(sys.argv[2]);
	else :
		usage_for_daikon();
	return;                                             

#This is r_tool runner script

def runner():
	command_string = "runner.py run";
	os.system(command_string);
	return;

def dtrace_manip(counter):
	command_string = "cp -r tr_gen/*.dtrace arijit/"
	os.system(command_string);
	os.chdir("arijit");
	for file in os.listdir("."):
		if not "_" in file: 
			file_names = file.split(".");
			assert len(file_names) == 2;
			final_name =  file_names[0]+"_"+str(counter)+"."+file_names[1];
			os.rename(file,final_name)
	os.chdir("../");
	return 

def compiling() :
	command_string = "runner.py comp"
	os.system(command_string);
	return ;

def load_and_run():
	command_string = "runner.py load_only"
	os.system(command_string);
	return ;

def tr_clean() :
	command_string = "runner.py clean"
	os.system(command_string);
	return ;

def r_tool_usage() :
	help_string = " First run tukli.py comp then change the program points if needed\n \
			Then run the tukli.py run #, where # is the run counter \n" +  \
			"Remember to set the program points everytime you run";
	print help_string;
        return 

def all_clean():

	command_string = "rm -rf *";
	if os.path.exists("arijit"):
		os.chdir("arijit");
		os.system(command_string);
		os.chdir("../");
		
	if os.path.exists("tr_gen"):
		os.chdir("tr_gen");
		os.system(command_string);
		os.chdir("../");

	return

def main_rtool():
	init();
	if len(sys.argv) < 2 :
		r_tool_usage();
		return ;
	
	if sys.argv[1] == "comp" :
		compiling();

	elif sys.argv[1] == "run" :
		assert len(sys.argv) == 3;
		counter =  sys.argv[2];
		load_and_run();
		dtrace_manip(counter);

	elif sys.argv[1] == "clean":
		all_clean();

	else :
		r_tool_usage();

	return ;
#This is the critical section finding code

def cs_tool_usage():
	help_msg = "The idea is to first generate the static program points i.e. those that are related only the functions.\n Then compile the program and run\
			To compile tukli.py comp\n To run tukli.py run <#> <$> # is the run_count $ is the sp_count\n To clean the previous results run tukli clean"
	print help_msg;
	return;

#def cs_compiling(counter):
        #command_string = "runner.py comp_sp "+str(counter);

def cs_compiling():
	command_string = "runner.py gen_ppt"
	os.system(command_string);
	return ;

def cs_runner(sp_count):
	command_string = "runner.py loadsp_only "+str(sp_count);
	os.system(command_string);
	return;

def cs_clean():
	command_string = "rm -rf *";
	if os.path.exists("arijit"):
		os.chdir("arijit");
		os.system(command_string);
		os.chdir("../");
		
	if os.path.exists("tr_gen"):
		os.chdir("tr_gen");
		os.system(command_string);
		os.chdir("../");
	
	return 
	
def main_cs():

	init();
	if  len(sys.argv) < 2 :
		cs_tool_usage();
		return;
	
	if sys.argv[1] == "comp" :
		#assert len(sys.argv) == 3;
		#counter = sys.argv[2];
		#cs_compiling(counter);
		shutil.copy(daikon_path+"/hook.h", ".");
		cs_compiling();

	elif sys.argv[1] == "run" :
		print "Length is : "+str(len(sys.argv));
		assert len(sys.argv) == 4;
		run_counter =  sys.argv[2];
		sp_count = sys.argv[3];
	       	cs_runner(sp_count);
	       	dtrace_manip(run_counter)

	elif sys.argv[1] == "clean":
		cs_clean();

	else :
		cs_tool_usage();
	return 



#CS Finding Code ends here



if __name__ == "__main__" :
	main();
