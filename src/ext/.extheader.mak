ext.d: ../../include/v3ExtType.h ../../include/v3ExtUtil.h ../../include/v3Formula.h ../../include/v3LTLFormula.h ../../include/v3NtkElaborate.h ../../include/v3Property.h 
../../include/v3ExtType.h: v3ExtType.h
	@rm -f ../../include/v3ExtType.h
	@ln -fs ../src/ext/v3ExtType.h ../../include/v3ExtType.h
../../include/v3ExtUtil.h: v3ExtUtil.h
	@rm -f ../../include/v3ExtUtil.h
	@ln -fs ../src/ext/v3ExtUtil.h ../../include/v3ExtUtil.h
../../include/v3Formula.h: v3Formula.h
	@rm -f ../../include/v3Formula.h
	@ln -fs ../src/ext/v3Formula.h ../../include/v3Formula.h
../../include/v3LTLFormula.h: v3LTLFormula.h
	@rm -f ../../include/v3LTLFormula.h
	@ln -fs ../src/ext/v3LTLFormula.h ../../include/v3LTLFormula.h
../../include/v3NtkElaborate.h: v3NtkElaborate.h
	@rm -f ../../include/v3NtkElaborate.h
	@ln -fs ../src/ext/v3NtkElaborate.h ../../include/v3NtkElaborate.h
../../include/v3Property.h: v3Property.h
	@rm -f ../../include/v3Property.h
	@ln -fs ../src/ext/v3Property.h ../../include/v3Property.h
