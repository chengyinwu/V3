io.d: ../../include/v3NtkParser.h ../../include/v3NtkWriter.h 
../../include/v3NtkParser.h: v3NtkParser.h
	@rm -f ../../include/v3NtkParser.h
	@ln -fs ../src/io/v3NtkParser.h ../../include/v3NtkParser.h
../../include/v3NtkWriter.h: v3NtkWriter.h
	@rm -f ../../include/v3NtkWriter.h
	@ln -fs ../src/io/v3NtkWriter.h ../../include/v3NtkWriter.h
