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
RM = rm

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
# cmake doesn't seem to support parallel under *BSD
.PHONY: cmake
cmake: $(VERSFN)
	@VERS=$$(cat $(VERSFN)); \
	case $$(uname -s) in \
          Linux) \
	    COMP=$(GCC) \
	    VERS=$${VERS} \
	    $(MAKE) cmake-unix; \
            pmode=--parallel $(MAKE) build; \
            ;; \
	  *BSD) \
	    COMP=$(CLANG) \
	    VERS=$${VERS} \
	    $(MAKE) cmake-unix; \
	    $(MAKE) build; \
            ;; \
	  Darwin) \
	    COMP=$(CLANG) \
	    VERS=$${VERS} \
	    $(MAKE) cmake-unix; \
	    pmode=--parallel $(MAKE) build; \
            ;; \
	  MINGW*) \
	    COMP=$(GCC) \
	    VERS=$${VERS} \
	    $(MAKE) cmake-windows; \
	    $(MAKE) build; \
            ;; \
	esac

.PHONY: cmakeclang
cmakeclang: $(VERSFN)
	@VERS=$$(cat $(VERSFN)); \
	case $$(uname -s) in \
          Linux) \
	    COMP=$(CLANG) \
	    VERS=$${VERS} \
	    $(MAKE) cmake-unix; \
            pmode=--parallel $(MAKE) build; \
            ;; \
	  *BSD) \
	    COMP=$(CLANG) \
	    VERS=$${VERS} \
	    $(MAKE) cmake-unix; \
	    $(MAKE) build; \
            ;; \
	  Darwin) \
	    COMP=$(CLANG) \
	    VERS=$${VERS} \
	    $(MAKE) cmake-unix; \
	    pmode=--parallel $(MAKE) build; \
            ;; \
	  MINGW*) \
	    COMP=/mingw64/bin/clang.exe \
	    VERS=$${VERS} \
	    $(MAKE) cmake-windows; \
	    $(MAKE) build; \
            ;; \
	esac

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

.PHONY: doc
doc: $(VERSFN)
	VERS=$$(cat $(VERSFN)); \
	doxygen libmp4tag.doxygen

# source

# the wiki/ directory has the changelog in it
.PHONY: sourcetar
sourcetar: $(VERSFN)
	@$(MAKE) tclean
	VERS=$$(cat $(VERSFN)); \
	TARGZ=libmp4tag-src-$${VERS}.tar.gz; \
	TDIR=libmp4tag-$${VERS}; \
	test -f $${TARGZ} && $(RM) -f $${TARGZ}; \
	test -d $${TDIR} && $(RM) -rf $${TDIR}; \
	mkdir $${TDIR}; \
	cp -pfr \
		*.c *.h CMakeLists.txt Makefile config.h.in \
		libmp4tag.doxygen \
		DEVNOTES.txt README.txt LICENSE.txt \
		wiki \
		$${TDIR}; \
	tar -c -z -f libmp4tag-src-$${VERS}.tar.gz $${TDIR}; \
	$(RM) -rf $${TDIR}

# cleaning

.PHONY: distclean
distclean:
	@-$(MAKE) tclean
	@-$(RM) -rf build tmp doxygen man
	@-$(RM) -f libmp4tag-src-*.tar.gz
	@mkdir $(BUILDDIR)

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-test -d build && cmake --build build --target clean

.PHONY: tclean
tclean:
	@-$(RM) -f w ww www *~ core *.orig *.doxygen.bak
	@-$(RM) -f wiki/*~ dev/*~
