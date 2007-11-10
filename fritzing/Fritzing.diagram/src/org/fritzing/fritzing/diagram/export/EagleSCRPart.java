package org.fritzing.fritzing.diagram.export;


public class EagleSCRPart {
	/* replicate_schematic.scr - type syntax:
	 * 		ADD LED@fritzing-0001a 'LED1' 'G$1' R0.000 (1.050 1.450);
	 * what it needs: 
	 * start with "ADD"
	 * Library part and library - e.g. LED@fritzing-0001a
	 * 		from *.fritzing / *.fritzing_diagram where it is formatted as "xsi:type="fritzing:LED""
	 * Part name - e.g. 'LED1'
	 * 		from *.fritzing / *.fritzing_diagram where it is formatted as "id="L1""
	 * 		referenced in *.fbb where it takes the form "href="default.fritzing#L1""
	 * Gate number - e.g. 'G$1' 
	 * 		not important for now - can hopefully constrain library parts to only use G$1 and auto-fill here
	 * Rotation - e.g. R90.000
	 * 		front-end does not currently implement - will just fill with R0.000 for now (think it has to be three decimal places)
	 * Coordinates - e.g. (1.050 1.450)
	 * 		found in *.fbb in the layoutConstraint element, as a sibling of "element" which contains the part name in format "href="default.fritzing#R1""
	 * Semicolon to close it out
	 */
	
	public String partType = "";
	public String libraryName = "";
	public String partName = "";
	public String gateNumber = "G$1";
	public String rotationPrefix = "R";
	public String rotationVal = "0.000";
	public CoordPair fritzingPartPos = new CoordPair();
	public CoordPair fritzingPartPosInch = new CoordPair();
	public CoordPair partPos = new CoordPair();
	public int locked = 0;			// should the part's position be locked?
	
	public EagleSCRPart(String partName, String partType, String libraryName, CoordPair fritzingPartPos) {
		super();
		this.partName = partName;
		this.partType = partType; 
		this.libraryName = libraryName;
		this.fritzingPartPos = fritzingPartPos;
		this.fritzingPartPosInch = new CoordPair((float)((fritzingPartPos.xVal/1000) / 2.54), (float)((fritzingPartPos.yVal/1000) / 2.54));
		this.partPos = fritzingPartPosInch;
	}
	
	public void setPosition(CoordPair partPos) {
		this.partPos = partPos;
	}
	
	public CoordPair getTerminalCoords(String terminal) {
		CoordPair result = new CoordPair(); 
		double xOffset = 0;
		double yOffset = 0;
		
		if (partType.equalsIgnoreCase("arduino")) {			
			if (terminal.equals("RESET")) {
				xOffset = 2.7;
				yOffset = 1.4;
			}
			if (terminal.equals("3V3")) {
				xOffset = 2.7;
				yOffset = 1.5;
			}
			if (terminal.equals("5V")) {
				xOffset = 2.7;
				yOffset = 1.6;
			}
			if (terminal.equals("GND")) {
				xOffset = 2.7;
				yOffset = 1.8;
			}
		}
		if (partType.equalsIgnoreCase("resistor")) {
			if (terminal.equals("0")) {
				xOffset = -0.2;
				yOffset = 0;
			}
			if (terminal.equals("1")) {
				xOffset = 0.2;
				yOffset = 0;
			}
		}
		if (partType.equalsIgnoreCase("led")) {
			
		}
		if (partType.equalsIgnoreCase("button")) {
			
		}
		result.xVal = partPos.xVal + (float)xOffset;
		result.yVal = partPos.yVal + (float)yOffset;
		return result;
	}
	
	// we provide the 'locked' flag and a few methods to control the so-called "locked" status
	// of a part.  some parts, once placed, should not be moved (e.g. the Arduino in very simple cases,
	// or buttons, LEDs, etc. if a user would like them in a particular place)
	// this lock is soft - the class' elements do not respect the lock flag so it is the responsibility
	// of the calling routine to check
	public int posIsLocked() {
		return this.locked;
	}
	
	public void lockPos() {
		locked = 1;
	}
}
