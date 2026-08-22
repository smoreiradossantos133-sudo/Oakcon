PROJECT_DIR := OakOS

.PHONY: all kernel iso run run-gui run-vnc stop-vnc debug test clean

all kernel iso run run-gui run-vnc stop-vnc debug test clean:
	$(MAKE) -C $(PROJECT_DIR) $@
