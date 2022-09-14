cmd.d: ../../include/v3CmdComm.h ../../include/v3CmdMgr.h 
../../include/v3CmdComm.h: v3CmdComm.h
	@rm -f ../../include/v3CmdComm.h
	@ln -fs ../src/cmd/v3CmdComm.h ../../include/v3CmdComm.h
../../include/v3CmdMgr.h: v3CmdMgr.h
	@rm -f ../../include/v3CmdMgr.h
	@ln -fs ../src/cmd/v3CmdMgr.h ../../include/v3CmdMgr.h
