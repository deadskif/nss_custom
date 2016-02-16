MAJOR		= 0
MINOR		= 2
VERSION		= $(MAJOR).$(MINOR)
PACKET		= nss_config
SOVERSION	= 2


LIBNAME		= lib$(PACKET).so
LIBVERSION	= $(LIBNAME).$(MAJOR).$(MINOR)
SONAME		= $(LIBNAME).$(SOVERSION)

RELEASE ?= yes
ifeq ($(DEBUG),yes)
RELEASE = no
endif

CC		?= $(CROSS_COMPILE)gcc
AS		?= $(CROSS_COMPILE)gcc -x assembler-with-cpp
LD		?= $(CROSS_COMPILE)ld
OBJCOPY	?= $(CROSS_COMPILE)objcopy
AR		?= $(CROSS_COMPILE)ar

RM	?= rm
HOSTCC	?= gcc
INSTALL ?= install

# From GNU Coding standarts
# 7.2.3 Variables for Specifying Commands
# 7.2.4 DESTDIR: Support for Staged Installs
# 7.2.5 Variables for Installation Directories

prefix 		?=	/usr/local
exec_prefix ?=	$(prefix)
bindir		?=	$(exec_prefix)/bin
datarootdir ?=  $(prefix)/share
includedir	?=	$(prefix)/include
docdir		?=  $(datarootdir)/doc/$(PACKET)
libdir		?=  $(exec_prefix)/lib
srcdir		?=	$(CURDIR)

INSTALL_PROGRAM ?= $(INSTALL)
INSTALL_DATA 	?= $(INSTALL) -m 644

ifeq ($(RELEASE),yes)
CFLAGS = -Wall
else
CFLAGS = -Wall -Werror
endif

LDFLAGS = -Wl,-O1 -Wl,--as-needed -shared -Wl,-soname,$(SONAME)
CFLAGS += -fPIC -O2 
# -std=c99 confilcts with sturct mreqn
CFLAGS += -pedantic
CFLAGS += -std=c99

CFLAGS += -I$(srcdir)/include

ifeq ($(RELEASE),no)
CFLAGS += -g -ggdb
endif

export
.PHONY: all clean help test install

all: $(LIBVERSION)
$(LIBVERSION): nss_custom.c Makefile
	$(CC) $(CFLAGS) $(LDFLAGS) $< -o $@
	ln -s $@ $(SONAME)
	ln -s $@ $(LIBNAME)

test: nss_custom_test.c Makefile all
	$(CC) $(CFLAGS) -l$(PACKET) $< -o $@

help:
	@echo 'Building userspace'
	@echo 'Targets:'
	@echo ' make'
	@echo ' make all		- Build all targets'
	@echo ' make clean		- Clean all targets'

clean:
	@echo "Clean"
	@-rm -f test_base
	@find . -name "*.o" -exec rm {} \;
	@find . -name "*.d" -exec rm {} \;
	@find . -name "*.so.*" -exec rm {} \;
	@find . -name "*.so" -exec rm {} \;	
	@find . -name "*.a" -exec rm {} \;	

install: installdirs all
	@$(INSTALL_PROGRAM) $(LIBVERSION) $(DESTDIR)$(libdir)/ && \
	ln -s $(LIBVERSION) $(DESTDIR)$(libdir)/$(SONAME) && \
	ln -s $(LIBVERSION) $(DESTDIR)$(libdir)/$(LIBNAME) ; done
# Make sure all installation directories (e.g. $(bindir))
# actually exist by making them if necessary.
installdirs:
	@for dir in \
            $(DESTDIR)$(libdir) \
		do if [ ! -d "$$dir" ]; then mkdir -p "$$dir"; fi; done


