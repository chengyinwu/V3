dfx.d: ../../include/v3DfxCmd.h ../../include/v3DfxSimplify.h ../../include/v3DfxTrace.h 
../../include/v3DfxCmd.h: v3DfxCmd.h
	@rm -f ../../include/v3DfxCmd.h
	@ln -fs ../src/dfx/v3DfxCmd.h ../../include/v3DfxCmd.h
../../include/v3DfxSimplify.h: v3DfxSimplify.h
	@rm -f ../../include/v3DfxSimplify.h
	@ln -fs ../src/dfx/v3DfxSimplify.h ../../include/v3DfxSimplify.h
../../include/v3DfxTrace.h: v3DfxTrace.h
	@rm -f ../../include/v3DfxTrace.h
	@ln -fs ../src/dfx/v3DfxTrace.h ../../include/v3DfxTrace.h
