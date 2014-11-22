ntk.d: ../../include/v3NtkCmd.h ../../include/v3NtkHandler.h ../../include/v3NtkInput.h ../../include/v3NtkSimplify.h ../../include/v3NtkUtil.h ../../include/v3Ntk.h ../../include/v3NtkHash.h ../../include/v3NtkRewrite.h ../../include/v3NtkStrash.h ../../include/v3Type.h 
../../include/v3NtkCmd.h: v3NtkCmd.h
	@rm -f ../../include/v3NtkCmd.h
	@ln -fs ../src/ntk/v3NtkCmd.h ../../include/v3NtkCmd.h
../../include/v3NtkHandler.h: v3NtkHandler.h
	@rm -f ../../include/v3NtkHandler.h
	@ln -fs ../src/ntk/v3NtkHandler.h ../../include/v3NtkHandler.h
../../include/v3NtkInput.h: v3NtkInput.h
	@rm -f ../../include/v3NtkInput.h
	@ln -fs ../src/ntk/v3NtkInput.h ../../include/v3NtkInput.h
../../include/v3NtkSimplify.h: v3NtkSimplify.h
	@rm -f ../../include/v3NtkSimplify.h
	@ln -fs ../src/ntk/v3NtkSimplify.h ../../include/v3NtkSimplify.h
../../include/v3NtkUtil.h: v3NtkUtil.h
	@rm -f ../../include/v3NtkUtil.h
	@ln -fs ../src/ntk/v3NtkUtil.h ../../include/v3NtkUtil.h
../../include/v3Ntk.h: v3Ntk.h
	@rm -f ../../include/v3Ntk.h
	@ln -fs ../src/ntk/v3Ntk.h ../../include/v3Ntk.h
../../include/v3NtkHash.h: v3NtkHash.h
	@rm -f ../../include/v3NtkHash.h
	@ln -fs ../src/ntk/v3NtkHash.h ../../include/v3NtkHash.h
../../include/v3NtkRewrite.h: v3NtkRewrite.h
	@rm -f ../../include/v3NtkRewrite.h
	@ln -fs ../src/ntk/v3NtkRewrite.h ../../include/v3NtkRewrite.h
../../include/v3NtkStrash.h: v3NtkStrash.h
	@rm -f ../../include/v3NtkStrash.h
	@ln -fs ../src/ntk/v3NtkStrash.h ../../include/v3NtkStrash.h
../../include/v3Type.h: v3Type.h
	@rm -f ../../include/v3Type.h
	@ln -fs ../src/ntk/v3Type.h ../../include/v3Type.h
