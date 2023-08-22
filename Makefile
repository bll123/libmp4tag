#!/usr/bin/make
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

MAKEFLAGS += --no-print-directory

BUILDDIR = build
GCC = gcc
CLANG = clang
VERSFN = tmp/vers.txt
PREFIX = /usr

.PHONY: release
release:
	LIBMP4TAG_BUILD=Release $(MAKE) cmake

.PHONY: debug
debug:
	LIBMP4TAG_BUILD=Debug $(MAKE) cmake

.PHONY: debug-clang
debug-clang:
	LIBMP4TAG_BUILD=Debug $(MAKE) cmakeclang

.PHONY: sanitizeaddress
sanitizeaddress:
	LIBMP4TAG_BUILD=SanitizeAddress $(MAKE) cmake

.PHONY: sanitizeaddressclang
sanitizeaddressclang:
	LIBMP4TAG_BUILD=SanitizeAddress $(MAKE) cmakeclang

.PHONY: cmake
.PHONY: cmakeclang cmake-unix cmake-windows

$(VERSFN): libmp4tag.h
	@MAJVERS=$$(grep '^#define LIBMP4TAG_VERS_MAJOR [0-9]' libmp4tag.h \
		| sed 's,.* ,,'); \
	MINVERS=$$(grep '^#define LIBMP4TAG_VERS_MINOR [0-9]' libmp4tag.h \
		| sed 's,.* ,,'); \
	REVVERS=$$(grep '^#define LIBMP4TAG_VERS_REVISION [0-9]' libmp4tag.h \
		| sed 's,.* ,,'); \
	VERS="$${MAJVERS}.$${MINVERS}.$${REVVERS}"; \
	test -d tmp || mkdir tmp; \
	echo $${VERS} > $(VERSFN)

# parallel doesn't seem to work under msys2
.PHONY: cmake
cmake: $(VERSFN)
	@VERS=$$(cat $(VERSFN)); \
	if test $$(uname -s) = Linux; then \
	  COMP=$(GCC) \
	  VERS=$${VERS} \
	  $(MAKE) cmake-unix; \
          pmode=--parallel $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  COMP=$(CLANG) \
	  VERS=$${VERS} \
	  $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  COMP=$(GCC) \
	  VERS=$${VERS} \
	  $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

.PHONY: cmakeclang
cmakeclang: $(VERSFN)
	@VERS=$$(cat $(VERSFN)); \
	if test $$(uname -s) = Linux; then \
	  COMP=$(CLANG) \
	  VERS=$${VERS} \
	  $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	elif test $$(uname -s) = Darwin; then \
	  COMP=$(CLANG) \
	  VERS=$${VERS} \
	  $(MAKE) cmake-unix; \
	  pmode=--parallel $(MAKE) build; \
	else \
	  COMP=/mingw64/bin/clang.exe \
	  VERS=$${VERS} \
	  $(MAKE) cmake-windows; \
	  $(MAKE) build; \
	fi

.PHONY: cmake-unix
cmake-unix:
	cmake \
		-DCMAKE_C_COMPILER=$(COMP) \
		-DLIBMP4TAG_BUILD:STATIC=$(LIBMP4TAG_BUILD) \
		-DLIBMP4TAG_BUILD_VERS:STATIC=$(VERS) \
		-S . -B $(BUILDDIR) -Werror=deprecated

.PHONY: cmake-windows
cmake-windows:
	cmake \
		-DCMAKE_C_COMPILER="$(COMP)" \
		-DLIBMP4TAG_BUILD:STATIC=$(LIBMP4TAG_BUILD) \
		-DLIBMP4TAG_BUILD_VERS:STATIC=$(VERS) \
		-G "MSYS Makefiles" \
		-S . -B $(BUILDDIR) -Werror=deprecated

# cmake on windows installs extra unneeded crap
# --parallel does not work correctly on msys2
.PHONY: build
build:
	cmake --build $(BUILDDIR) $(pmode)

.PHONY: install
install:
	cmake --install $(BUILDDIR) --prefix "$(PREFIX)"

# source

# the wiki/ directory has the changelog in it
.PHONY: sourcetar
sourcetar: $(VERSFN)
	@$(MAKE) tclean
	@-$(RM) -f libmp4tag-src-*.tar.gz
	VERS=$$(cat $(VERSFN)); \
	tar -c -z -f libmp4tag-src-$${VERS}.tar.gz \
		--exclude wikibearer.txt \
		*.c *.h CMakeLists.txt Makefile config.h.in \
		DEVNOTES.txt README.txt LICENSE.txt \
		wiki

# cleaning

.PHONY: distclean
distclean:
	@-$(MAKE) tclean
	@-$(RM) -rf build tmp
	@-$(RM) -f libmp4tag-src-*.tar.gz
	@mkdir $(BUILDDIR)

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-test -d build && cmake --build build --target clean

.PHONY: tclean
tclean:
	@-$(RM) -f w ww www *~ core *.orig
	@-$(RM) -f wiki/*~ dev/*~
