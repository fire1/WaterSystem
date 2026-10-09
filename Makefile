# Host unit tests for Master and Slave (no Arduino toolchain required).
.PHONY: test clean

test:
	$(MAKE) -C Master/tests test
	$(MAKE) -C Slave/tests test

clean:
	$(MAKE) -C Master/tests clean
	$(MAKE) -C Slave/tests clean
