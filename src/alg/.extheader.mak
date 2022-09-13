alg.d: ../../include/v3AlgSimulate.h ../../include/v3AlgType.h 
../../include/v3AlgSimulate.h: v3AlgSimulate.h
	@rm -f ../../include/v3AlgSimulate.h
	@ln -fs ../src/alg/v3AlgSimulate.h ../../include/v3AlgSimulate.h
../../include/v3AlgType.h: v3AlgType.h
	@rm -f ../../include/v3AlgType.h
	@ln -fs ../src/alg/v3AlgType.h ../../include/v3AlgType.h
