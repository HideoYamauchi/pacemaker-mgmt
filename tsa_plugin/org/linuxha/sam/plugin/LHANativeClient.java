package org.linuxha.sam.plugin;

public class LHANativeClient extends LHALIBClient {
	synchronized String[] processCommand(String[] command)
	{
		String msg = "";
		for(int i=0; i<command.length; i++){
			msg += command[i];
			msg += " ";
		}
		String result = LHAMgmtLib.process_cmnd_native(msg);
		return result.split("\n");
	}
}
