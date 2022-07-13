# source codes to be compiled
SRCPKGS  = cmd vlp syn ckt int util

# library to be linked (should include SRCPKGS)
LIBPKGS  = cmd vlp syn ckt int util

MAIN     = main

LIBS     = $(addprefix -l, $(LIBPKGS))
SRCLIBS  = $(addsuffix .a, $(addprefix lib, $(SRCPKGS)))
LIBFILES = $(addsuffix .a, $(addprefix lib, $(LIBPKGS)))

EXEC     = qutertl
QLIB     = libquteRTL.a
AR       = ar cr

all: srcLib
	@echo "Checking $(MAIN)..."
	@cd src/$(MAIN); make --no-print-directory INCLIB="$(LIBS) -lz -lgmp -lstdc++ -lreadline -ltermcap" EXEC=$(EXEC); cd ../.. ;
	@ln -fs bin/$(EXEC) .
#	@strip bin/$(EXEC)
       	
srcLib: 
	@for pkg in $(SRCPKGS); \
	do \
		echo "Checking $$pkg..."; \
		cd src/$$pkg; make --no-print-directory PKGNAME=$$pkg; \
		cd ../.. ; \
	done

v3Sed: 
	@for pkg in $(SRCPKGS); \
	do \
		echo "Check V3 Compatibility of $$pkg..."; \
		cd src/$$pkg; sed -i 's/\<Msg\>/QuteMsg/g' *.h; sed -i 's/\<Msg\>/QuteMsg/g' *.cpp; \
		sed -i 's/\<MSG\>/QuteMSG/g' *.h; sed -i 's/\<MSG\>/QuteMSG/g' *.cpp; \
		sed -i 's/PREFIX_CHAR \"k\"/PREFIX_CHAR \"QuteRTL_Internal\"/g' *.h; \
		sed -i 's/PREFIX_CHAR \"k\"/PREFIX_CHAR \"QuteRTL_Internal\"/g' *.cpp; \
		cd ../.. ; \
	done

v3SedOff: 
	@for pkg in $(SRCPKGS); \
	do \
		echo "Check QuteRTL Compatibility of $$pkg..."; \
		cd src/$$pkg; sed -i 's/\<QuteMsg\>/Msg/g' *.h; sed -i 's/\<QuteMsg\>/Msg/g' *.cpp; \
		sed -i 's/\<QuteMSG\>/MSG/g' *.h; sed -i 's/\<QuteMSG\>/MSG/g' *.cpp; \
		sed -i 's/PREFIX_CHAR \"QuteRTL_Internal\"/PREFIX_CHAR \"k\"/g' *.h; \
		sed -i 's/PREFIX_CHAR \"QuteRTL_Internal\"/PREFIX_CHAR \"k\"/g' *.cpp; \
		cd ../.. ; \
	done

quteLib: srcLib
	@echo "Building $(QLIB)..."
	@for pkg in $(LIBPKGS); \
	do \
		cd src/$$pkg; cp -r *.o ../../obj; \
		cd ../.. ; \
	done
	@$(AR) $(QLIB) obj/*.o
	@ranlib $(QLIB)
	@cp -f src/int/*.h .
	@rm -f obj/*

v3Lib: v3Sed quteLib v3SedOff

clean:
	@for pkg in $(SRCPKGS); \
	do \
		echo "Cleaning $$pkg..."; \
		cd src/$$pkg; make --no-print-directory PKGNAME=$$pkg clean; \
		cd ../.. ; \
	done
	@echo "Cleaning $(MAIN)..."
	@cd src/$(MAIN); make --no-print-directory clean
	@echo "Removing $(LIBFILES)..."
	@cd lib; rm -f $(LIBFILES)
	@echo "Removing $(EXEC)..."
	@rm -f bin/$(EXEC) 
	@echo "Removing $(QLIB)..."
	@rm -f $(QLIB)
	@echo "Removing obj/"
	@cd obj/; rm -rf *.o

ctags:	  
	@rm -f src/tags
	@for pkg in $(SRCPKGS); \
	do \
		echo "Tagging $$pkg..."; \
		cd src; ctags -a $$pkg/*.cpp $$pkg/*.h; cd ..; \
	done
	@echo "Tagging $(MAIN)..."
	cd src; ctags -a $(MAIN)/*.cpp
cleandep:
	@for pkg in $(SRCPKGS); \
	do \
		rm src/$$pkg/.*.mak; \
	done
	@rm src/main/.*.mak;
