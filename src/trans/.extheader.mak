trans.d: ../../include/v3BvBlastAig.h ../../include/v3BvBlastAigHelper.h ../../include/v3BvBlastBv.h ../../include/v3TransCmd.h 
../../include/v3BvBlastAig.h: v3BvBlastAig.h
	@rm -f ../../include/v3BvBlastAig.h
	@ln -fs ../src/trans/v3BvBlastAig.h ../../include/v3BvBlastAig.h
../../include/v3BvBlastAigHelper.h: v3BvBlastAigHelper.h
	@rm -f ../../include/v3BvBlastAigHelper.h
	@ln -fs ../src/trans/v3BvBlastAigHelper.h ../../include/v3BvBlastAigHelper.h
../../include/v3BvBlastBv.h: v3BvBlastBv.h
	@rm -f ../../include/v3BvBlastBv.h
	@ln -fs ../src/trans/v3BvBlastBv.h ../../include/v3BvBlastBv.h
../../include/v3TransCmd.h: v3TransCmd.h
	@rm -f ../../include/v3TransCmd.h
	@ln -fs ../src/trans/v3TransCmd.h ../../include/v3TransCmd.h
