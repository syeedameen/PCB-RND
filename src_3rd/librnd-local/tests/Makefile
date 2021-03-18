test:
	cd rnd_printf && $(MAKE) all && $(MAKE) test
	cd remote && $(MAKE) all && $(MAKE) test
# This uses make instead of $(MAKE) on purpose as a "portbale" -j1 to work aroudn GNU make
	cd librnd && make all && make test
	@echo " "
	@echo "+-------------------------------------------------+"
	@echo "+  All tests passed, librnd is safe to install.  +"
	@echo "+-------------------------------------------------+"

clean:
	cd rnd_printf && $(MAKE) clean
	cd remote && $(MAKE) clean
	cd librnd && $(MAKE) clean

