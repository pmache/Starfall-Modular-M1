# Windows convenience entrypoint; build.ps1 stages unique, verified packages.
TOOLCHAIN ?= V:/dev/msys64/mingw64
.DEFAULT_GOAL := all
all:
	powershell.exe -NoProfile -ExecutionPolicy Bypass -File ./build.ps1 -Toolchain "$(TOOLCHAIN)"
test: all
grab: all
.PHONY: all test grab
