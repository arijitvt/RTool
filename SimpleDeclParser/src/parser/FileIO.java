package parser;

import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.io.LineNumberReader;
import java.io.Reader;
import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public class FileIO {
	private String fileName;
	private FileReader fileReader = null;
	private DaikonLineNumberReader lineNumberReader = null;
	private Set<String> threadSet = new HashSet<String>();
	List<String> dummyArrayList = new ArrayList<String>();
	
	
	private String declContainerFileName = "output.decl";
	
	//Debug vars
	boolean flag = false;

	/**
	 * Constructor
	 * 
	 * @param fileName
	 */
	public FileIO(String fileName) {
		this.setFileName(fileName);
		dummyArrayList.add("");
	}

	/**
	 * 
	 * Getters and Setters
	 */

	public String getFileName() {
		return fileName;
	}

	public void setFileName(String fileName) {
		this.fileName = fileName;
	}

	/**
	 * Parser Implementation
	 * 
	 * @throws FileFormatException
	 * @throws IOException
	 */

	public void read_decl_file() throws FileFormatException, IOException {
		try {
			fileReader = new FileReader(fileName);
			System.out.println("Inpuit file: " + fileName);
			// bufferedReader = new BufferedReader(fileReader);
			lineNumberReader = new DaikonLineNumberReader(fileReader);
			List<String> declData = new ArrayList<String>();
			String line = "";
			boolean flag = false;
			do {
                                // TODO: lineNumberReader can returns null at
                                // the end of a file. How should this case be
                                // handled?
				line = lineNumberReader.readLine();			
                                assert line != null;
				if (line.startsWith("input-language") && !flag) {
					declData.add(line);
					flag = true;
				} else if (flag) {
					declData.add(line);
				} else {
					String exceptionMsg = "Error at "
							+ lineNumberReader.getLineNumber();
					throw new FileFormatException(exceptionMsg);
				}
			} while (!line.trim().equals(""));

			flag = false;
			// Write the decl file's static info
			write_date_to_file(declContainerFileName, false, declData);
			declData.clear();

                        // loop over the input file until we reach the ine
                        // which starts with input-language
                        for (;;) {
				line = lineNumberReader.readLine();
                                // lineNumberReader appears to return NULL at the end of the stream
                                if (line == null) {
                                  break;
                                }
				System.out.println("The line is: " + line + " :at line number "+lineNumberReader.getLineNumber());
				if (line.startsWith("ppt")) {
					declData.add(line);
					flag = true;
				} else if (line.startsWith("input-language")) {
                                        // the exit condition of the loop is reaching
                                        // the line which starts with input-language
					flag = false;
                                        System.err.println("[DEBUG] Breaking out of read decl file loop");
					break;
				} else if (flag) {
					declData.add(line);

				} else {
					String exceptionMsg = "Error at "
							+ lineNumberReader.getLineNumber();
					throw new FileFormatException(exceptionMsg);
				}
                        }

			write_date_to_file(declContainerFileName, true, declData);
			declData.clear();
		} catch (IOException ioException) {
			ioException.printStackTrace();
		} finally {
			lineNumberReader.close();
			fileReader.close();
		}

	}

	/**
	 * 
	 * @throws FileFormatException
	 * @throws IOException
	 */
	public void read_dtrace_file() throws FileFormatException, IOException {
		try {
                        System.err.println("[DEBUG] reading decl file: " + fileName);
			fileReader = new FileReader(fileName);
			lineNumberReader = new DaikonLineNumberReader(fileReader);
			String line = lineNumberReader.readLine();
			// Skip the decl part
			do {
				line = lineNumberReader.readLine();
			} while (line != null && !line.startsWith("input-language"));

			Reader: while (lineNumberReader.ready()) {
				// Now We will Skip few More locations to get actual reading
				// place
				while (!line.trim().equals("") && lineNumberReader.ready()) {
					line = lineNumberReader.readLine();
				}

				List<String> data = new ArrayList<String>();
				while (!line.startsWith("input-language")
						&& lineNumberReader.ready()) {
					line = lineNumberReader.readLine();
					if (line.contains("Bogus_Trace")) {
						data.clear();
						line  = skipToNextSection(lineNumberReader);
					}
					if (!line.startsWith("input-language")) {
						data.add(line);
					}
				}
				if(flag) {
					flag = false;
					continue;
				}
				// Now Our Data Has the entire trace for one single run
//				displayList(data);
				List<String> threadBlockTrace = new ArrayList<String>();
				for (int i = 0; i < data.size(); ++i) {
					while (i < data.size() && !data.get(i).trim().equals("")) {
						threadBlockTrace.add(data.get(i));
						++i;
					}
					processThreadBlockTrace(threadBlockTrace);
					threadBlockTrace.clear();
				}
			}
		} catch (FileNotFoundException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} catch (CloneNotSupportedException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
			lineNumberReader.close();
			fileReader.close();
		}
	}

	/**
	 * 
	 * @param threadBlockTrace
	 * @throws IOException
	 */
	private void processThreadBlockTrace(List<String> threadBlockTrace)
			throws IOException {
		// TODO Auto-generated method stub
		String threadID = threadBlockTrace.get(0).split("::::")[1];
		String outputFileName = threadID + ".dtrace";
		threadBlockTrace.remove(0);
		// This is to give a line feed after a block
		if (threadSet.contains(threadID)) {
			write_date_to_file(outputFileName, true, threadBlockTrace);
			// linefeed
			write_date_to_file(outputFileName, true, dummyArrayList);
		} else {
			threadSet.add(threadID);
			copy_decl_to_dtrace(declContainerFileName, outputFileName);
			write_date_to_file(outputFileName, true, threadBlockTrace);
			// linefeed
			write_date_to_file(outputFileName, true, dummyArrayList);
		}
	}

	/**
	 * 
	 * @param decl_file_name
	 * @param dtrace_file_name
	 * @throws IOException
	 */
	private void copy_decl_to_dtrace(final String decl_file_name,
			final String dtrace_file_name) throws IOException {
		FileReader srcFileReader = new FileReader(decl_file_name);
		LineNumberReader lineNumberReader = new LineNumberReader(srcFileReader);

		List<String> declHeader = new ArrayList<String>();
		String line = lineNumberReader.readLine();

		// Collect the header information
		while (lineNumberReader.ready() && !line.trim().equals("")) {
			declHeader.add(line);
			line = lineNumberReader.readLine();
		}
		write_date_to_file(dtrace_file_name, false, declHeader);
		write_date_to_file(dtrace_file_name, true, dummyArrayList);

		// Collect Decl Information
		line = lineNumberReader.readLine();
		List<String> declInfo = new ArrayList<String>();
		while (lineNumberReader.ready()
				&& !line.trim().startsWith("input-language")) {
			declInfo.add(line);
			line = lineNumberReader.readLine();
		}
		write_date_to_file(dtrace_file_name, true, declInfo);
		write_date_to_file(dtrace_file_name, true, dummyArrayList);
		write_date_to_file(dtrace_file_name, true, declHeader);
		write_date_to_file(dtrace_file_name, true, dummyArrayList);

	}

	/**
	 * 
	 * @param fileName
	 * @param openMode
	 * @param data
	 * @throws IOException
	 */
	private void write_date_to_file(final String fileName,
			final boolean openMode, final List<String> data) throws IOException {
		FileWriter fileWriter = null;
		BufferedWriter bufferedWriter = null;
		try {
			fileWriter = new FileWriter(fileName, openMode);
			bufferedWriter = new BufferedWriter(fileWriter);
			for (String line : data) {
				if (!line.equals("")) {
					bufferedWriter.write(line);
					bufferedWriter.newLine();
				} else if (line.equals("")) {
					bufferedWriter.newLine();
				}
			}
		} catch (IOException e) {
			// TODO Auto-generated catch block
			e.printStackTrace();
		} finally {
			bufferedWriter.close();
			fileWriter.close();

		}
	}

	private String skipToNextSection(DaikonLineNumberReader reader) throws IOException, CloneNotSupportedException {
		String line = "";
		do {
			line = reader.readLine();			
		} while (!line.contains("input-language") && reader.ready());
		flag = true;
		return line;
	}
	
//	private void skipToNextSection(DaikonLineNumberReader reader) throws IOException, CloneNotSupportedException {
//		DaikonLineNumberReader newReader = new DaikonLineNumberReader(reader.clone());
//		String line = "";
//		do {
//			line = newReader.readLine();
//			if (!line.contains("input-language")) {
//				reader = newReader.clone();
//			}
//		} while (!line.contains("input-language") && newReader.ready());
//		flag = true;
//	}

	// Helper Functions
	private void displayList(List<String> list) {
		for (String s : list) {
			System.out.println(s);
		}
	}
	
}
