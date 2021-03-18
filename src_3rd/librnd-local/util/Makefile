# plain old hand crafted Makefile
ROOT=..

all:

include $(ROOT)/Makefile.conf

clean:

distclean:

install_all:
#	$(SCCBOX) mkdir -p  "$(BINDIR)" "$(LIBDIR)"

install:
	$(MAKE) install_all HOW="install -f"

linstall:
	$(MAKE) install_all HOW="linstall -f"

uninstall:
	$(MAKE) install_all HOW="uninstall"

