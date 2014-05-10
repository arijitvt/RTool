#!/usr/bin/env python

import os;
import sys;
import shutil;

rtool = "r_tool_ui.py"
java  = "java"
blank = " "

#inspect paths

def gen_ppts():
	command_string=rtool+blank+" --dump-points ."
	os.system(command_string);
	return ;

def do_instrumentation() :
	command_string = rtool+blank+" --run .";
	os.system(command_string);
	return;

def do_instrumentation_sp(sp_val) :
	command_string = rtool+blank+" --run_sp . "+str(sp_val);
	os.system(command_string);
	return;

def runner(choice,count,argument) :                               
        command_string = "";
	choice = int(choice);
     	print "The choice is  coming as :"+str(choice);   
        if choice is 0 :
        	command_string = "inspect  ./a.out "+argument;
        
        elif choice is 1:
            command_string = "inspect --max-pcb "+str(count)+" ./a.out  "+argument ;

        elif choice is 2:                                               
            command_string= "inspect --max-pset "+str(count)+"  ./a.out "+argument;

	print "The command string is : "+command_string;
	os.system(command_string);
	return;

def parser() :
	flag = False;
	for file in  os.listdir(".") :
		if os.path.isdir(file) and file.startswith("tr_gen"):
			flag = True;
			break

	if not flag:
		os.system("mkdir -p tr_gen");

	shutil.copy("program.dtrace","tr_gen/program.dtrace1");
	os.chdir("tr_gen");
	command_string = java+" parser.Main "+\
					" --filename program.dtrace1";
	os.system(command_string);
	os.system(" java daikon.Daikon *.dtrace > daikon_result");
	os.system (" cat daikon_result");
	os.chdir("../");

	return;     

def only_parser():

	flag = False;
	for file in  os.listdir(".") :
		if os.path.isdir(file) and file.startswith("tr_gen"):
			flag = True;
			break

	if not flag:
		os.system("mkdir -p tr_gen");

	shutil.copy("program.dtrace","tr_gen/program.dtrace1");
	os.chdir("tr_gen");
	command_string = java+"  parser.Main "+\
					" --filename program.dtrace1";
	os.system(command_string);
	os.chdir("../");

	   

		
def cleanup():
	os.system("rm -rf program.dtrace");
	os.chdir("tr_gen")
	os.system("rm -rf *");
	os.chdir("../");
	if os.path.exists("result"):
		os.chdir("result")
		os.system("rm -rf *");
		os.chdir("../");

def usage() :
	print "Enter <runner.py run> to run everything"
	print "Enter <runner.py run_sp <spcount #>> to run everything"
	print "Enter <runner.py load> to load the ppt and do everything.\n It does not generate the ppt"
	print "Enter <runner.py load_sp <spcount #>> Same as above but with sp count facility."
	print "Enter <runner.py load_only> to load the ppt and do everything.\n It does not generate the ppt and do not run the daikon."
	print "Enter <runner.py comp> to gen ppts and compile "
	print "Enter <runner.py comp_sp <spcount #>> to run everything"
	print "Enter <runner.py comp_run> to just run the program not parser or daikon"
	print "Etner <runner.py clean> to clean previous trace"
	print "Enter <runner.py help> to get the help"
	print "Enter <runner.py intel> for intelligent run to generate critical section"



def main_controller() :


	if sys.argv[1] == "run":
		gen_ppts();
		do_instrumentation();
		runner();
		parser();

	elif sys.argv[1] == "load":
		do_instrumentation();
		runner();
		parser();


	elif sys.argv[1] == "load_only":
		print "------------------------- Load Only -----------------"
		do_instrumentation();
		alg_choice= sys.argv[2];
		alg_count=sys.argv[3];
		arg=" ";
		runner(alg_choice,alg_count,arg);
		only_parser();

#	elif sys.argv[1] == "load_sp":
#		if len(sys.argv) < 3:
#			usage();
#			exit(0)
#
#		else:
#			sp_val = sys.argv[2];
#			do_instrumentation_sp(sp_val);
#			runner();
#			parser();
#
#	elif sys.argv[1] == "loadsp_only":
#	        if len(sys.argv) < 3:
#	       		usage();
#		       	exit(0)
#
#	        else:
#			sp_val = sys.argv[2];
#			do_instrumentation_sp(sp_val);
#			runner();
#			only_parser();
#
#	elif sys.argv[1] == "run_sp":
#		if len(sys.argv) < 3:
#			usage();
#			exit(0)
#
#		else:
#			sp_val = sys.argv[2];
#			gen_ppts();
#			do_instrumentation_sp(sp_val);
#			runner();
#			parser();
#
#
#	elif sys.argv[1] == "gen_ppt":
#		gen_ppts();
#
	elif sys.argv[1] == "comp":
		gen_ppts();
		do_instrumentation();
#	
#	elif sys.argv[1] == "comp_sp":
#		if len(sys.argv) < 3:
#			usage();
#			exit(0)
#
#		else:
#			sp_val = sys.argv[2];
#			print "SpValue is : "+str(sp_val);
#			gen_ppts();
#			do_instrumentation_sp(sp_val);
#
#	elif sys.argv[1] == "comp_run":
#		runner();
#
#	elif sys.argv[1] == "clean":
#		cleanup();

#	elif sys.argv[1] == "intel":
#		do_intelligent_run();

	elif sys.argv[1] == "help":
		usage();
	else:
		usage();

	return;


if __name__ == "__main__":
	main_controller();

