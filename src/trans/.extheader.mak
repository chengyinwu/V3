trans.d: ../../include/v3BvBlastAig.h ../../include/v3BvBlastAigHelper.h ../../include/v3BvBlastBv.h ../../include/v3NtkExpand.h ../../include/v3NtkFlatten.h ../../include/v3NtkMiter.h ../../include/v3TransCmd.h 
../../include/v3BvBlastAig.h: v3BvBlastAig.h
	@rm -f ../../include/v3BvBlastAig.h
	@ln -fs ../src/trans/v3BvBlastAig.h ../../include/v3BvBlastAig.h
../../include/v3BvBlastAigHelper.h: v3BvBlastAigHelper.h
	@rm -f ../../include/v3BvBlastAigHelper.h
	@ln -fs ../src/trans/v3BvBlastAigHelper.h ../../include/v3BvBlastAigHelper.h
../../include/v3BvBlastBv.h: v3BvBlastBv.h
	@rm -f ../../include/v3BvBlastBv.h
	@ln -fs ../src/trans/v3BvBlastBv.h ../../include/v3BvBlastBv.h
../../include/v3NtkExpand.h: v3NtkExpand.h
	@rm -f ../../include/v3NtkExpand.h
	@ln -fs ../src/trans/v3NtkExpand.h ../../include/v3NtkExpand.h
../../include/v3NtkFlatten.h: v3NtkFlatten.h
	@rm -f ../../include/v3NtkFlatten.h
	@ln -fs ../src/trans/v3NtkFlatten.h ../../include/v3NtkFlatten.h
../../include/v3NtkMiter.h: v3NtkMiter.h
	@rm -f ../../include/v3NtkMiter.h
	@ln -fs ../src/trans/v3NtkMiter.h ../../include/v3NtkMiter.h
../../include/v3TransCmd.h: v3TransCmd.h
	@rm -f ../../include/v3TransCmd.h
	@ln -fs ../src/trans/v3TransCmd.h ../../include/v3TransCmd.h
