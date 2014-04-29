package parser;

import java.io.LineNumberReader;
import java.io.Reader;

public class DaikonLineNumberReader extends LineNumberReader implements
		Cloneable {

	public DaikonLineNumberReader(Reader in) {
		super(in);
		// TODO Auto-generated constructor stub
	}
	
	public DaikonLineNumberReader clone() throws CloneNotSupportedException {
		return (DaikonLineNumberReader) super.clone();
	}

}
