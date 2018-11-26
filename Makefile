TOPDIR	=	.
include config.mk

SRCDIR		= src
SUBDIRS		= $(foreach dir,$(wildcard */Makefile),$(dir $(dir)))

MODULESLIBS	= $(addsuffix .a,$(MODULES))


.PHONY: all clean install
.PHONY: $(SUBDIRS)
.SUFFIXES:

all: $(HEXFILE) $(BINFILE)
	@echo 
	@echo "Build complete."
	@echo 

install:
	stty -F $(TTYDEV) $(TTYBAUD)
	cat $(NAME).hex > $(TTYDEV)

clean: $(SUBDIRS)
	@echo " [ RM ] $(BINFILE) $(HEXFILE) $(ELFFILE) $(ELFDEBUGFILE)"
	@$(RM) $(BINFILE) $(HEXFILE) $(ELFFILE) $(ELFDEBUGFILE)
	
	@echo
	@echo "Source tree cleaned."
	@echo

$(ELFFILE): $(ELFDEBUGFILE)
	@echo " [STRP] $@"
	@$(TARGET)strip -o $@ $<

$(ELFDEBUGFILE): $(SUBDIRS)
	@echo " [ LD ] $@"
	@$(CC) -o $@ $(CFLAGS) -Wl,--whole-archive $(addsuffix /out.a,$(SRCDIR)) $(MODULESLIBS) -Wl,--no-whole-archive $(LDFLAGS)

$(HEXFILE): $(ELFFILE)
	@echo " [OCPY] $@"
	@$(TARGET)objcopy -O ihex $< $@
	@$(SREC_CAT) -O $@ -Intel $@ -Intel

$(BINFILE): $(ELFFILE)
	@echo " [OCPY] $@"
	@$(TARGET)objcopy -O binary $< $@

$(SUBDIRS):
	@echo " [ CD ] $(CURRENTPATH)$@"
	@+make -C "$@" "CURRENTPATH=$(CURRENTPATH)$@" $(MAKECMDGOALS)
