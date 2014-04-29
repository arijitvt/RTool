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
			do{
				if(!lineReader.ready()) {
					break;
				}
				line = lineReader.readLine();
				if(!line.startsWith("input-language")) {
					dataList.add(line);	
				}
			}while(lineReader.ready() && !line.startsWith("input-language"));
			write_date_to_file(outputFileName, false, dataList);
			while(lineReader.ready()) {
				dataList.clear();
				readBlock(lineReader,dataList);
				ArrayList<Block> blockList = new ArrayList<Block>();
				generateBlockList(dataList,blockList);				
				ArrayList<Block> result =processBlockData(blockList);
				dataList.clear();
				generateDataStreamFromBlockStream(result, dataList);
				writePrivateInfo(outputFileName, true);
				write_date_to_file(outputFileName, true, dataList);
			}
		} catch (FileNotFoundException e) {
			e.printStackTrace();
		} catch (IOException e) {
			e.printStackTrace();
		}finally {
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
	
	
	private void generateBlockList(ArrayList<String> dataList,ArrayList<Block> blockList) {
		for(int i = 0 ; i < dataList.size(); ++i) {
			String line  = "";
			Block b = new Block();
			do{
				line = dataList.get(i).trim();
				if(!line.equals("")) {
					b.blockInfo.add(dataList.get(i));
					++i;
				}		
			}while(!line.equals("") && i < dataList.size());
			b.processBlockInformation();
			blockList.add(b);
		}
	}

	private ArrayList<Block> processBlockData(ArrayList<Block> blockList) {
		ArrayList<Block> output = new ArrayList<Block>();
		for(int i = 0 ;  i < blockList.size(); ++i) {
			Block srcBlock = blockList.get(i);
			if(srcBlock.getCallStackCounter() == 0) {
				output.add(srcBlock);
			}else if(srcBlock.isEntryOrExit()){
				for(int j = i ; j < blockList.size(); ++j) {
					if((blockList.get(j).getFunctionName().equals(srcBlock.getFunctionName())) &&
							(!blockList.get(j).isEntryOrExit()) &&
							srcBlock.getThreadId().equals(blockList.get(j).getThreadId())){
						output.add(srcBlock);
						output.add(blockList.get(j));
						break;
					}
				}
			}
		}
//		System.out.println("Output list size is "+output.size());
//		showBlockList(output);
		return output;
	}

	private void readBlock(DaikonLineNumberReader lineReader,
			ArrayList<String> dataList) throws IOException {
		// TODO Auto-generated method stub
		String line = "";
		do {
			if(!lineReader.ready()) {
				break;
			}
			line = lineReader.readLine().trim();
		}while(!line.equals(""));//Skipping done
		
		do {
			if(!lineReader.ready()) {
				break;
			}
			line = lineReader.readLine();
			if(!line.startsWith("input-language")) {
				dataList.add(line);
			}
 			
		}while(lineReader.ready() && !line.startsWith("input-language"));
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
			e.printStackTrace();
		} finally {
			bufferedWriter.close();
			fileWriter.close();

		}
	}

	
	private void generateDataStreamFromBlockStream(List<Block> blockList,List<String> dataList) {
		for(Block b :blockList) {
			dataList.add(b.toString());
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
			e.printStackTrace();
		} finally {
			bufferedWriter.close();
			fileWriter.close();

		}
	}

}
