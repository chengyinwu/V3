PKGFLAG	=
EXTHDRS	= 

include ../Makefile.in

TARGET	= ../../$(EXEC)

target: $(TARGET)

$(TARGET): $(COBJS) $(LIBDEP)
	@echo "> building $(EXEC)..."
	@$(CC) $(CFLAGS) -I$(INCDIR) $(COBJS) -L$(LIBDIR) $(EXTLIB) -static -lpthread -o $@
#	@$(CC) $(CFLAGS) -I$(INCDIR) $(COBJS) -L$(LIBDIR) $(EXTLIB) -lpthread -o $@
