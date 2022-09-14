v3mc.d: ../../include/v3MCCmd.h ../../include/v3MCEngine.h ../../include/v3MCMain.h 
../../include/v3MCCmd.h: v3MCCmd.h
	@rm -f ../../include/v3MCCmd.h
	@ln -fs ../src/v3mc/v3MCCmd.h ../../include/v3MCCmd.h
../../include/v3MCEngine.h: v3MCEngine.h
	@rm -f ../../include/v3MCEngine.h
	@ln -fs ../src/v3mc/v3MCEngine.h ../../include/v3MCEngine.h
../../include/v3MCMain.h: v3MCMain.h
	@rm -f ../../include/v3MCMain.h
	@ln -fs ../src/v3mc/v3MCMain.h ../../include/v3MCMain.h
