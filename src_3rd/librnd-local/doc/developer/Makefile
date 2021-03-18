ROOT=../..
DEVDIR=$(DOCDIR)/developer

all:

install_all:
	$(SCCBOX) mkdir -p $(DEVDIR)/hid_remote
	$(SCCBOX) $(HOW) hid_remote/*.html hid_remote/*.svg $(DEVDIR)/hid_remote/

install:
	$(MAKE) install_all HOW="install -f -d"

linstall:
	$(MAKE) install_all HOW="install -f -l -d"

uninstall:
	$(MAKE) install_all HOW="install -u"

clean:

distclean:


include $(ROOT)/Makefile.conf
