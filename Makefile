#!/usr/bin/make
#
# Copyright 2023 Brad Lanam Pleasant Hill CA
#

MAKEFLAGS += --no-print-directory

BUILDDIR = build
GCC = gcc
CLANG = clang
LOCTMP = tmp
VERSFN = $(LOCTMP)/vers.txt
SRCFLAG = $(LOCTMP)/source.txt
PFXFN = $(LOCTMP)/prefix.txt
RM = rm

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
	@test -d $(LOCTMP) || mkdir $(LOCTMP)
	@MAJVERS=$$(grep '^#define LIBMP4TAG_VERS_MAJOR [0-9]' libmp4tag.h \
		| sed 's,.* ,,'); \
	MINVERS=$$(grep '^#define LIBMP4TAG_VERS_MINOR [0-9]' libmp4tag.h \
		| sed 's,.* ,,'); \
	REVVERS=$$(grep '^#define LIBMP4TAG_VERS_REVISION [0-9]' libmp4tag.h \
		| sed 's,.* ,,'); \
	VERS="$${MAJVERS}.$${MINVERS}.$${REVVERS}"; \
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
	  *) \
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
	    COMP=/ucrt64/bin/clang.exe \
	    VERS=$${VERS} \
	    $(MAKE) cmake-windows; \
	    $(MAKE) build; \
            ;; \
	  *) \
	    COMP=clang \
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
# --parallel also seems to not work on *BSD
.PHONY: build
build:
	cmake --build $(BUILDDIR) $(pmode)

# force cmake to re-build the pkgconfig file
# make sure the correct prefix is available for the pkgconfig file.
# cmake's --prefix argument is disassociated from the prefix needed
# for the pkgconfig file.
.PHONY: install
install: $(VERSFN)
	@$(RM) -f libmp4tag.pc $(PFXFN)
	@if [ "$(PREFIX)" = "" ]; then echo "No prefix set"; exit 1; fi
	@echo $(PREFIX) > $(PFXFN)
	cmake --install $(BUILDDIR) --prefix "$(PREFIX)"
	@$(RM) -f libmp4tag.pc $(PFXFN)

# source

# the wiki/ directory has the changelog in it
.PHONY: source
source: $(VERSFN)
	@VERS=$$(cat $(VERSFN)); \
	SRCDIR=libmp4tag-$${VERS}; \
	$(MAKE) tclean; \
	$(MAKE) SRCDIR=$${SRCDIR} $(SRCFLAG); \
	$(MAKE) VERS=$${VERS} SRCDIR=$${SRCDIR} sourcetar; \
	$(MAKE) VERS=$${VERS} SRCDIR=$${SRCDIR} sourcezip; \
	$(RM) -rf $${SRCDIR} $(SRCFLAG)

$(SRCFLAG):
	@test -d $(LOCTMP) || mkdir $(LOCTMP)
	@if [ "$(SRCDIR)" = "" ]; then echo "No source-dir set"; exit 1; fi
	@-test -d $(SRCDIR) && $(RM) -rf $(SRCDIR)
	@mkdir $(SRCDIR)
	@cp -pfr \
		*.c *.h CMakeLists.txt Makefile config.h.in libmp4tag.pc.in \
		README.txt LICENSE.txt \
		tests wiki \
		$(SRCDIR)
	@touch $(SRCFLAG)

.PHONY: sourcetar
sourcetar: $(SRCFLAG)
	TARGZ=libmp4tag-src-$(VERS).tar.gz; \
	test -f $${TARGZ} && $(RM) -f $${TARGZ}; \
	tar -c -z -f $${TARGZ} $(SRCDIR)

.PHONY: sourcezip
sourcezip: $(SRCFLAG)
	ZIPF=libmp4tag-src-$(VERS).zip; \
	test -f $${ZIPF} && $(RM) -f $${ZIPF}; \
	zip -q -o -r -9 $${ZIPF} $(SRCDIR)

# cleaning

.PHONY: distclean
distclean:
	@-$(MAKE) tclean
	@-$(RM) -rf build $(LOCTMP)
	@-$(RM) -f libmp4tag-src-[0-9]*[0-9].[tz]* libmp4tag.pc
	@mkdir $(BUILDDIR)

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-test -d build && cmake --build build --target clean
	@$(RM) -f libmp4tag.pc

.PHONY: tclean
tclean:
	@-$(RM) -f w ww www core
	@-$(RM) -f *~ */*~ *.orig */*.orig
