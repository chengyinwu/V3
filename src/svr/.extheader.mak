svr.d: ../../include/v3SvrBase.h ../../include/v3SvrBoolector.h ../../include/v3SvrHandler.h ../../include/v3SvrMiniSat.h ../../include/v3SvrSatHelper.h ../../include/v3SvrType.h 
../../include/v3SvrBase.h: v3SvrBase.h
	@rm -f ../../include/v3SvrBase.h
	@ln -fs ../src/svr/v3SvrBase.h ../../include/v3SvrBase.h
../../include/v3SvrBoolector.h: v3SvrBoolector.h
	@rm -f ../../include/v3SvrBoolector.h
	@ln -fs ../src/svr/v3SvrBoolector.h ../../include/v3SvrBoolector.h
../../include/v3SvrHandler.h: v3SvrHandler.h
	@rm -f ../../include/v3SvrHandler.h
	@ln -fs ../src/svr/v3SvrHandler.h ../../include/v3SvrHandler.h
../../include/v3SvrMiniSat.h: v3SvrMiniSat.h
	@rm -f ../../include/v3SvrMiniSat.h
	@ln -fs ../src/svr/v3SvrMiniSat.h ../../include/v3SvrMiniSat.h
../../include/v3SvrSatHelper.h: v3SvrSatHelper.h
	@rm -f ../../include/v3SvrSatHelper.h
	@ln -fs ../src/svr/v3SvrSatHelper.h ../../include/v3SvrSatHelper.h
../../include/v3SvrType.h: v3SvrType.h
	@rm -f ../../include/v3SvrType.h
	@ln -fs ../src/svr/v3SvrType.h ../../include/v3SvrType.h
