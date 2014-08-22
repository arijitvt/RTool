package parser;

import java.io.File;
import java.io.FileWriter;
import java.io.IOException;
import java.io.FilenameFilter;

public class Main {
	
	public static void show_help() {
		System.out.println("Type java -jar parser.jar --filename <dtrace_filename>");		
	}

	public static  File[] finder(String dirName) {
		File dir = new File(dirName);

		return dir.listFiles(new FilenameFilter() {
			public boolean accept(File dir, String filename) {
				return filename.endsWith(".dtrace");
			}
		});
	}
	public static void cleanup() {
		File fileList[] = finder(".");
		for (File file : fileList) {
			try {
				FileWriter fileWriter = new FileWriter(file);
				fileWriter.close();
			} catch (IOException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}
	}
	
	public static void main(String args[]) {
		if(args.length != 2) {
			show_help();
			return;
		}
		String input_dtrace_file="";
		if(args[0].equals("--filename")) {
			input_dtrace_file = args[1];
		}
		System.out.println("Input Dtracefile file: "+ input_dtrace_file);
		Restruct rs = new Restruct(input_dtrace_file);
		String intermediate_file = rs.doRestructure();
		FileIO fileIO = new FileIO(intermediate_file);
//		cleanup();
		try {
			fileIO.read_decl_file();
			fileIO.read_dtrace_file();
			System.out.println("\n-----Decl Parser Stage is Done----");			
		} catch (FileFormatException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		} 
	}

}
