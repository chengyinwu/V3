alg.d: ../../include/v3AlgCmd.h ../../include/v3AlgGeneralize.h ../../include/v3AlgSim.h ../../include/v3AlgSimulate.h ../../include/v3AlgType.h 
../../include/v3AlgCmd.h: v3AlgCmd.h
	@rm -f ../../include/v3AlgCmd.h
	@ln -fs ../src/alg/v3AlgCmd.h ../../include/v3AlgCmd.h
../../include/v3AlgGeneralize.h: v3AlgGeneralize.h
	@rm -f ../../include/v3AlgGeneralize.h
	@ln -fs ../src/alg/v3AlgGeneralize.h ../../include/v3AlgGeneralize.h
../../include/v3AlgSim.h: v3AlgSim.h
	@rm -f ../../include/v3AlgSim.h
	@ln -fs ../src/alg/v3AlgSim.h ../../include/v3AlgSim.h
../../include/v3AlgSimulate.h: v3AlgSimulate.h
	@rm -f ../../include/v3AlgSimulate.h
	@ln -fs ../src/alg/v3AlgSimulate.h ../../include/v3AlgSimulate.h
../../include/v3AlgType.h: v3AlgType.h
	@rm -f ../../include/v3AlgType.h
	@ln -fs ../src/alg/v3AlgType.h ../../include/v3AlgType.h
