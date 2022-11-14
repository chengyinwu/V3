int.d: ../../include/quteRTL.h 
../../include/quteRTL.h: quteRTL.h
	@rm -f ../../include/quteRTL.h
	@ln -fs ../src/int/quteRTL.h ../../include/quteRTL.h
