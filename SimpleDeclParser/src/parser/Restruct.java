package parser;

import java.awt.DisplayMode;
import java.io.BufferedWriter;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.FileWriter;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

public class Restruct {
	private String fileName="";
	private String outputFileName ="arijit.output";
	public Restruct(String fileName) {
		this.fileName = fileName;
	}

	public String getOutputFileName() {
		return outputFileName;
	}

	public String doRestructure() {
		FileReader fileReader = null;
		DaikonLineNumberReader lineReader = null;
		ArrayList<String> dataList = new ArrayList<String>();
		try {
			fileReader = new FileReader(fileName);
			lineReader = new DaikonLineNumberReader(fileReader);
			//Read A block
			String line = lineReader.readLine();
			dataList.add(line);

                        // markus: read the decl header on the file?
			do {
				if(!lineReader.ready()) {
                                        System.err.println("[DEBUG] doRestructure(): lineReader not ready");
					break;
				}
				line = lineReader.readLine();
				if(!line.startsWith("input-language")) {
					dataList.add(line);	
				}
			} while(lineReader.ready() && !line.startsWith("input-language"));
                        System.err.println("[DEBUG] Exited first loop in doRestructure(); lineReader.ready() ? "
                            + (lineReader.ready() ? "true" : "false"));
                        System.err.println("[DEBUG] on line " + lineReader.getLineNumber() + " of file " + fileName);

			write_data_to_file(outputFileName, false, dataList);

                        // markus: create individual dtrace files for each thread?
			while(lineReader.ready()) {
                                // Clear old data
				dataList.clear();

                                System.err.println("[DEBUG] reading block?");
				readBlock(lineReader,dataList);

				ArrayList<Block> blockList = new ArrayList<Block>();
                                System.err.println("[DEBUG] generating blocklist?");
				generateBlockList(dataList,blockList);				

                                System.err.println("[DEBUG] processing block?");
				ArrayList<Block> result = processBlockData(blockList);
				dataList.clear();

                                System.err.println("[DEBUG] generating data stream?");
                                // Given the list of blocks, turn it into a
                                // list of strings
				generateDataStreamFromBlockStream(result, dataList);

                                System.err.println("[DEBUG] writing private info?");
				writePrivateInfo(outputFileName, true);
                                System.err.println("[DEBUG] writing date?");
				write_data_to_file(outputFileName, true, dataList);
			}
                        System.err.println("[DEBUG] Exited second loop in doRestructure(); lineReader.ready() ? "
                            + (lineReader.ready() ? "true" : "false"));
                        System.err.println("[DEBUG] on line " + lineReader.getLineNumber() + " of file " + fileName);
		} 
                catch (FileNotFoundException e) {
			e.printStackTrace();
		} 
                catch (IOException e) {
			e.printStackTrace();
		} 
                finally {
			try {
				lineReader.close();
				fileReader.close();
				return outputFileName;
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
		return outputFileName;
	}
	
	
        // markus: NOTE: This is just my understanding of the code.
        // the passed datalist consists of blocks each of which is tagged with
        // a ThreadId value. The goal of this function is the iterate over the
        // data list and collect all the blocks.
	private void generateBlockList(ArrayList<String> dataList,ArrayList<Block> blockList) {
                // Assumption: Each block is separated by a blank line
                /*
		for(int i = 0 ; i < dataList.size(); ++i) {
			String line  = "";
			Block b = new Block();
			do {
				line = dataList.get(i).trim();
				if(!line.equals("")) {
					b.blockInfo.add(dataList.get(i));
					++i;
				}		
			} while(!line.equals("") && i < dataList.size());
			b.processBlockInformation();
			blockList.add(b);
		}
                */
                // The first block read from dataList. This will be inserted
                // into the block list. A new one will be created whenever we
                // reach a new line
                Block b = new Block();
                for (String line : dataList) {
                  line = line.trim();
                  if (!line.equals("")) {
                    b.blockInfo.add(line);
                  }
                  else {
                    // hitting a newline means we have reached the start of a
                    // new block. We do not need to add the newline (line) to
                    // the block.
                    b.processBlockInformation();
                    blockList.add(b);
                    // We have added the block to the list so we can create a
                    // new one now to prepare for the next block
                    b = new Block();
                  }
                }
                // collect all the blocks
	}

        // Given the passed list of blocks, place entry and exit blocks to the
        // same function by the same thread consecutively in a list
	private ArrayList<Block> processBlockData(ArrayList<Block> blockList) {
          ArrayList<Block> output = new ArrayList<Block>();
          // Iterate over all blocks in the block list
	  for (int i = 0 ;  i < blockList.size(); ++i) {
	    Block entryBlock = blockList.get(i);
            // TODO: Why is a block with callStackOunter == 0 always added?
            if (entryBlock.getCallStackCounter() == 0) {
              output.add(entryBlock);
            } 
            else if (entryBlock.isEntryOrExit()) {
              // EntryOrExit is true for entry blocks. Given an entry block
              // search forward in the blockList to find a matching exit
              // block for the srcBlock. If it is found, add both of them to
              // the result
              String entryFuncName = entryBlock.getFunctionName();
              String entryTId = entryBlock.getThreadId();
              for (int j = i ; j < blockList.size(); ++j) {
                Block b = blockList.get(j);
                if (!b.isEntryOrExit()) {
                  // block is to an exit function
                  String exitFuncName = b.getFunctionName();
                  if (entryFuncName.equals(exitFuncName)) {
                    String exitTId = b.getThreadId();
                    if (entryTId.equals(exitTId)) {
                      // same thread!
                      if ((entryBlock.getCallStackCounter() == b.getCallStackCounter())) {
                        output.add(entryBlock);
                        output.add(b);
                      }
                    }
                  }
                }
              } // for (int j = i...)
	    }  // else if (entryBlock.isEntryOrExit())
          } // for (int i = 0;...}
	  return output;
          //if ((blockList.get(j).getFunctionName().equals(srcBlock.getFunctionName())) 
          //    && (!blockList.get(j).isEntryOrExit()) 
          //    && srcBlock.getThreadId().equals(blockList.get(j).getThreadId())) {
          //  output.add(srcBlock);
          //  output.add(blockList.get(j));
          //  break;
          //}
	}

        // Read one run of the program. Each new run starts with
        // input-language. The entire run will contain the different function
        // entry/exit instrumentation outputs for multiple threads (meaning it
        // cannot be directly processed by Daikon).
	private void readBlock(DaikonLineNumberReader lineReader,
			ArrayList<String> dataList) throws IOException {
		String line = "";
                // Skip blank lines before the start of the block
		do {
			if(!lineReader.ready()) {
				break;
			}
			line = lineReader.readLine().trim();
		} while(!line.equals(""));
		
                // Read until we hit input-language
		do {
			if(!lineReader.ready()) {
				break;
			}
			line = lineReader.readLine();
			if(!line.startsWith("input-language")) {
				dataList.add(line);
			}
 			
		} while(lineReader.ready() && !line.startsWith("input-language"));

                if (!dataList.get(0).startsWith("ThreadId")) {
                        System.err.println("[ERROR] readBlock(): Malformed block does not start with ThreadID, line: " + dataList.get(0));
                }
                System.err.println("[DEBUG] readBlock(): first line of block: " + dataList.get(0));
	}

	/**
	 * 
	 * @param fileName
	 * @param openMode
	 * @param data
	 * @throws IOException
         *
         * Dump the passed list of strings to the passed filename
	 */
	private void write_data_to_file(final String fileName,
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
			e.printStackTrace();
		} finally {
			bufferedWriter.close();
			fileWriter.close();

		}
	}

	
        // Given a list of blocks, turn it into a list of strings
	private void generateDataStreamFromBlockStream(List<Block> blockList,List<String> dataList) {
		for(Block b :blockList) {
			dataList.add(b.toString());
                        // separate each block with a newline
			dataList.add("\n");
		}
		
	}
	
	private void showList(List<String> elements) {
		for(String s : elements ) {
			System.out.println("Data: "+s);
		}
	}
	
	private void showBlockList(ArrayList<Block> blockList) {
		for(Block b : blockList) {
//			b.displayBlockInfo();
			b.displayBlockDetails();
		}
	}
	
        // Write the header to the passed file
	private void writePrivateInfo(final String fileName,
			final boolean openMode) throws IOException {
		FileWriter fileWriter = null;
		BufferedWriter bufferedWriter = null;

		/**
		 * Static information
		 * 
		 * input-language C/C++
		 * decl-version 2.0
		 * var-comparability none
		 */
		
		
		ArrayList<String> data = new ArrayList<String>();
		data.add("input-language C/C++");
		data.add("decl-version 2.0");
		data.add("var-comparability none");
		
		
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
			bufferedWriter.newLine();
		} catch (IOException e) {
                        System.err.println("[ERROR] unable to write private info: " + e.getMessage());
			e.printStackTrace();
                        System.exit(1);
		} finally {
			bufferedWriter.close();
			fileWriter.close();

		}
	}

}
