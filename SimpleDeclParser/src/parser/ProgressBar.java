package parser;

public class ProgressBar implements Runnable {
	private static int counter = 0 ;
	
	@Override
	public void run() {
		// TODO Auto-generated method stub
		for(;;) {
			int len  = Integer.toString(counter).length();
			for(int i = 0 ; i  < len; ++i) {
				System.out.print("\b");
			}
			System.out.print("=>"+(++counter));
			try {
				Thread.currentThread().sleep(1000);
			} catch (InterruptedException e) {
				// TODO Auto-generated catch block
				e.printStackTrace();
			}
		}

	}

}
