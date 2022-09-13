vrf.d: ../../include/v3VrfBase.h ../../include/v3VrfCmd.h ../../include/v3VrfResult.h ../../include/v3VrfUMC.h ../../include/v3VrfShared.h 
../../include/v3VrfBase.h: v3VrfBase.h
	@rm -f ../../include/v3VrfBase.h
	@ln -fs ../src/vrf/v3VrfBase.h ../../include/v3VrfBase.h
../../include/v3VrfCmd.h: v3VrfCmd.h
	@rm -f ../../include/v3VrfCmd.h
	@ln -fs ../src/vrf/v3VrfCmd.h ../../include/v3VrfCmd.h
../../include/v3VrfResult.h: v3VrfResult.h
	@rm -f ../../include/v3VrfResult.h
	@ln -fs ../src/vrf/v3VrfResult.h ../../include/v3VrfResult.h
../../include/v3VrfUMC.h: v3VrfUMC.h
	@rm -f ../../include/v3VrfUMC.h
	@ln -fs ../src/vrf/v3VrfUMC.h ../../include/v3VrfUMC.h
../../include/v3VrfShared.h: v3VrfShared.h
	@rm -f ../../include/v3VrfShared.h
	@ln -fs ../src/vrf/v3VrfShared.h ../../include/v3VrfShared.h
