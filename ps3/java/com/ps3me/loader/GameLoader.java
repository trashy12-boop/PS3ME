/* SPDX-License-Identifier: GPL-2.0-only */
package com.ps3me.loader;
import javax.microedition.midlet.MIDlet;
import com.sun.midp.installer.*;
import com.sun.midp.main.MIDletSuiteUtils;
import com.sun.midp.midlet.MIDletSuite;
import com.sun.midp.midletsuite.*;
import com.sun.midp.configurator.Constants;
/** Installs into a persistent per-game application directory supplied by native code. */
public final class GameLoader extends MIDlet implements Runnable,InstallListener {
    private boolean started;
    protected void startApp(){if(!started){started=true;new Thread(this).start();}}
    protected void pauseApp(){}
    protected void destroyApp(boolean unconditional){}
    public void run(){
        try {
            String url=getAppProperty("arg-0");
            if(url==null||!url.startsWith("file:///"))throw new IllegalArgumentException("Local JAR/JAD required");
            MIDletSuiteStorage storage=MIDletSuiteStorage.getMIDletSuiteStorage();
            int[] suites=storage.getListOfSuites();
            int id;
            if(suites.length>0)id=suites[0];
            else {
                FileInstaller installer=new FileInstaller();
                if(url.toLowerCase().endsWith(".jad"))
                    id=installer.installJad(url,Constants.INTERNAL_STORAGE_ID,false,false,this);
                else id=installer.installJar(url,null,Constants.INTERNAL_STORAGE_ID,false,false,this);
            }
            MIDletSuite suite=storage.getMIDletSuite(id,false);
            String declaration=suite.getProperty("MIDlet-1");
            if(declaration==null){suite.close();throw new IllegalArgumentException("No MIDlet-1 manifest entry");}
            String className=new MIDletInfo(declaration).classname;
            String title=suite.getProperty("MIDlet-Name");
            suite.close();
            System.out.println("[PS3ME] starting suite "+id+" class "+className);
            MIDletSuiteUtils.execute(id,className,title);
        } catch(Throwable error) {
            System.out.println("[PS3ME] launch failed: "+error);error.printStackTrace();
        }
        notifyDestroyed();
    }
    public boolean warnUser(InstallState state){return true;}
    public boolean confirmJarDownload(InstallState state){return true;}
    public void updateStatus(int status,InstallState state){System.out.println("[PS3ME installer] status="+status);}
    public boolean keepRMS(InstallState state){return true;}
    public boolean confirmAuthPath(InstallState state){return true;}
    public boolean confirmRedirect(InstallState state,String newLocation){return false;}
}
