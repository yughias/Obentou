package launcher;

import java.io.File;

public class RomItem {
    File romFile;
    File screenshotFile;
    long lastModifiedTime; 
    String systemName; 

    public RomItem(File romFile, File screenshotFile, long lastModifiedTime, String systemName) {
        this.romFile = romFile;
        this.screenshotFile = screenshotFile;
        this.lastModifiedTime = lastModifiedTime;
        this.systemName = systemName;
    }
}
