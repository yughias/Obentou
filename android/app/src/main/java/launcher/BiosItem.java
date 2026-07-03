package launcher;

import java.io.File;

public class BiosItem {
    File biosFile;
    long lastModifiedTime;
    String assignedSystem; 

    public BiosItem(File biosFile, long lastModifiedTime, String assignedSystem) {
        this.biosFile = biosFile;
        this.lastModifiedTime = lastModifiedTime;
        this.assignedSystem = assignedSystem;
    }
}
