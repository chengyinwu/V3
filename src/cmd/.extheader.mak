cmd.d: ../../include/v3CmdMgr.h 
../../include/v3CmdMgr.h: v3CmdMgr.h
	@rm -f ../../include/v3CmdMgr.h
	@ln -fs ../src/cmd/v3CmdMgr.h ../../include/v3CmdMgr.h
