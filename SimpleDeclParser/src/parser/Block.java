package parser;

import java.util.ArrayList;

public class Block {
	
	private static int CounterId =-1;
	private int blockId;
	private String threadId;
	private String  functionName;
        // Set to false for an EXIT function. Otherwise, set to true
	private boolean EntryOrExit; //True for entry False for exit
	private String invocationType;
	private int callStackCounter;

	public ArrayList<String> blockInfo = new ArrayList<String>();
	
	public Block() {
		++CounterId;
		this.blockId = CounterId;
	}
	
	public void displayBlockInfo() {
		System.out.println("Block Info Begins-------------------");
		for(String s :blockInfo) {
			System.out.println(s);
		}
		System.out.println("Block Info Ends---------------------");
	}
	
	public void displayBlockDetails() {
		System.out.println("Showing information about the block ------------------");
		System.out.println("Block Id : "+blockId);
		System.out.println("Thread id :"+threadId);
		System.out.println("Function Name  : "+functionName);
		System.out.println("Entry or Exit "+EntryOrExit);
		System.out.println("Invocation type : "+invocationType);
		System.out.println("Call Stack Counter : "+callStackCounter);
		System.out.println("Block Info ends ------------------\n");
	}
	
	public void processBlockInformation() {
		// First line must have the thread information
                System.err.println("[DEBUG] processBlockInformation(): processing block: " + blockInfo.get(0));
                System.err.println("[DEBUG] starts with ThreadId? " + (blockInfo.get(0).startsWith("ThreadId") ? "true" : "false"));
                // a block which does not start with ThreadId is malformed.
		if (!blockInfo.get(0).startsWith("ThreadId")) {
                  System.err.println("[ERROR] Malformed block does not start with ThreadID, line: " + blockInfo.get(0));
                  System.exit(1);
                }
		this.threadId = blockInfo.get(0);
                System.err.println("[DEBUG] processBlockInformation(): threadId: " + threadId);
		
		//Second line must have the funtion name
		assert(blockInfo.get(1).startsWith(".."));
//		System.out.println("Block name: "+blockInfo.get(1) +" : Block id "+this.blockId);
		String[] strarr = blockInfo.get(1).split(":::");
		assert(strarr.length == 2);
		this.functionName = strarr[0];
		if(strarr[1].startsWith("ENTER")) {
			EntryOrExit = true;
		}else if (strarr[1].startsWith("EXIT")) {
			EntryOrExit = false;
		}else {
			EntryOrExit= true;
		}
		
		//Third info should be invocation type
		assert(blockInfo.get(2).equals("this_invocation_nonce"));
		this.invocationType = blockInfo.get(2);
		
		//Call Stack Counter
		Integer counter = Integer.parseInt(blockInfo.get(3));
		this.callStackCounter= counter.intValue();
	}
	
	@Override
	public String toString() {
		String result=null;
		for(String s: blockInfo) {
			if(result == null) {
				result = s;
			}else {
				result += s;				
			}
			result+="\n";
		}
                // remove the extra newline from the last iteration
                result = result.substring(0, result.length()-1);
		return result;
	}


	
	
	/**
	 * Getters
	 * 
	 */
	public String getThreadId() {
		return threadId;
	}

	public String getFunctionName() {
		return functionName;
	}

	public boolean isEntryOrExit() {
		return EntryOrExit;
	}

	public String getInvocationType() {
		return invocationType;
	}

	public int getCallStackCounter() {
		return callStackCounter;
	}

	public  int getBlockId() {
		return blockId;
	}
	
}
