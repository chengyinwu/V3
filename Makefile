# source codes to be compiled
SRCPKGS	= cmd ntk alg stg ext io trans vrf dfx svr adt util v3mc

# library to be linked (should include SRCPKGS)
LIBPKGS	= vrf dfx stg trans alg ext ntk io ntk svr cmd adt util v3mc

# engine header files and libraries to be soft linked
ENGPKGS	+= minisat
ENGPKGS	+= boolector

# front-end Verilog parser
ENGPKGS	+= quteRTL

# specific directories
ENGSSRC	= eng
MAIN	= main

# libraries (if you come up with some readline undefined errors, try replacing -ltinfo with either -ltermcap or -lncurses)
EXTLIBS	= -lm -lz -lrt -lreadline -ltermcap -ldl -lgmp -lstdc++
SRCLIBS = $(addprefix -l, $(LIBPKGS)) $(addprefix -l, $(ENGPKGS))

# compile outputs
EXEC	= v3
LIB	= libv3.a

all:	srcLib
	@echo "Checking $(MAIN)..."
	@cd src/$(MAIN); make --no-print-directory EXTLIB="$(SRCLIBS) $(EXTLIBS)" EXEC=$(EXEC); cd ../.. ;
#	@strip $(EXEC)

srcLib:	engLib
	@for pkg in $(SRCPKGS); \
	do \
		echo "Checking $$pkg..."; \
		cd src/$$pkg; make --no-print-directory PKGNAME=$$pkg; \
		cd ../.. ; \
	done

engLib:	
	@for pkg in $(ENGPKGS);	\
	do \
		cd include ; ln -fs ../src/$(ENGSSRC)/$$pkg/* .; cd .. ; \
		cd lib ; ln -fs ../src/$(ENGSSRC)/$$pkg/*.a .; cd .. ; \
	done
	@rm -f include/*.a

clean:	
	@for pkg in $(SRCPKGS); \
	do \
		echo "Cleaning $$pkg..."; \
		cd src/$$pkg; make --no-print-directory PKGNAME=$$pkg clean; \
		cd ../.. ; \
	done
	@echo "Cleaning $(MAIN)..."
	@cd src/$(MAIN); make --no-print-directory clean
	@echo "Removing $(EXEC)..."
	@rm -f $(EXEC) 

ctags:	
	@rm -f src/tags
	@for pkg in $(SRCPKGS); \
	do \
		echo "Tagging $$pkg..."; \
		cd src; ctags -a $$pkg/*.cpp $$pkg/*.h; cd ..; \
	done
	@echo "Tagging $(MAIN)..."
	cd src; ctags -a $(MAIN)/*.cpp
	@echo "Tagging $(GUI)..."
	cd src; ctags -a $(GUI)/*.cpp

rmdep:	
	@for pkg in $(SRCPKGS); \
	do \
		rm src/$$pkg/.*.mak; \
	done
	@rm src/main/.*.mak;
