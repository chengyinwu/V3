stg.d: ../../include/v3StgCmd.h ../../include/v3StgExtract.h ../../include/v3StgFSM.h ../../include/v3StgSDG.h 
../../include/v3StgCmd.h: v3StgCmd.h
	@rm -f ../../include/v3StgCmd.h
	@ln -fs ../src/stg/v3StgCmd.h ../../include/v3StgCmd.h
../../include/v3StgExtract.h: v3StgExtract.h
	@rm -f ../../include/v3StgExtract.h
	@ln -fs ../src/stg/v3StgExtract.h ../../include/v3StgExtract.h
../../include/v3StgFSM.h: v3StgFSM.h
	@rm -f ../../include/v3StgFSM.h
	@ln -fs ../src/stg/v3StgFSM.h ../../include/v3StgFSM.h
../../include/v3StgSDG.h: v3StgSDG.h
	@rm -f ../../include/v3StgSDG.h
	@ln -fs ../src/stg/v3StgSDG.h ../../include/v3StgSDG.h
