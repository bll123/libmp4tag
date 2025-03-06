#!/usr/bin/make
#
# Copyright 2023-2024 Brad Lanam Pleasant Hill CA
#

MAKEFLAGS += --no-print-directory

BUILDDIR = build
CLANG = clang
LOCTMP = tmp
SRCFLAG = $(LOCTMP)/source.txt

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

# parallel doesn't seem to work under msys2
# cmake doesn't seem to support parallel under *BSD
.PHONY: cmake
cmake:
	@. ./VERSION.txt ; \
	case $$(uname -s) in \
          Linux|Darwin) \
	    $(MAKE) cmake-unix; \
            pmode=--parallel $(MAKE) build; \
            ;; \
	  *BSD) \
	    $(MAKE) cmake-unix; \
	    $(MAKE) build; \
            ;; \
	  MINGW*) \
	    $(MAKE) cmake-windows; \
	    $(MAKE) build; \
            ;; \
	  *) \
	    $(MAKE) cmake-unix; \
	    $(MAKE) build; \
            ;; \
	esac

.PHONY: cmakeclang
cmakeclang:
	@. ./VERSION.txt ; \
	case $$(uname -s) in \
          Linux|Darwin) \
	    $(MAKE) CC=clang cmake-unix; \
            pmode=--parallel $(MAKE) build; \
            ;; \
	  *BSD) \
	    CC=clang \
	    $(MAKE) CC=clang cmake-unix; \
	    $(MAKE) build; \
            ;; \
	  MINGW*) \
	    $(MAKE) CC=/ucrt64/bin/clang.exe cmake-windows; \
	    $(MAKE) build; \
            ;; \
	  *) \
	    $(MAKE) CC=clang cmake-windows; \
	    $(MAKE) build; \
            ;; \
	esac

# internal use
.PHONY: cmake-unix
cmake-unix:
	@if [ "$(PREFIX)" = "" ]; then echo "No prefix set"; exit 1; fi
	cmake \
		-DCMAKE_C_COMPILER=$(CC) \
		-DCMAKE_INSTALL_PREFIX="$(PREFIX)" \
		-DLIBMP4TAG_BUILD:STATIC=$(LIBMP4TAG_BUILD) \
		-S . -B $(BUILDDIR) -Werror=deprecated

# internal use
.PHONY: cmake-windows
cmake-windows:
	@if [ "$(PREFIX)" = "" ]; then echo "No prefix set"; exit 1; fi
	cmake \
		-DCMAKE_C_COMPILER=$(COMP) \
		-DCMAKE_INSTALL_PREFIX="$(PREFIX)" \
		-DLIBMP4TAG_BUILD:STATIC=$(LIBMP4TAG_BUILD) \
		-G "MSYS Makefiles" \
		-S . -B $(BUILDDIR) -Werror=deprecated

# cmake on windows installs extra unneeded crap
# --parallel does not work correctly on msys2
# --parallel also seems to not work on *BSD
.PHONY: build
build:
	cmake --build $(BUILDDIR) $(pmode)

.PHONY: install
install:
	cmake --install $(BUILDDIR)

# source

# the wiki/ directory has the changelog in it
.PHONY: tar
tar:
	@. ./VERSION.txt ; \
	SRCDIR=libmp4tag-$${LIBMP4TAG_VERSION} ; \
	$(MAKE) tclean ; \
	$(MAKE) SRCDIR=$${SRCDIR} $(SRCFLAG) ; \
	$(MAKE) SRCDIR=$${SRCDIR} sourcetar ; \
	$(MAKE) SRCDIR=$${SRCDIR} sourcezip ; \
	rm -rf $${SRCDIR} $(SRCFLAG)

# internal use
$(SRCFLAG):
	@test -d $(LOCTMP) || mkdir $(LOCTMP)
	@if [ "$(SRCDIR)" = "" ]; then echo "No source-dir set"; exit 1; fi
	@-test -d $(SRCDIR) && rm -rf $(SRCDIR)
	@mkdir $(SRCDIR)
	@cp -pfr \
		CMakeLists.txt Makefile \
		VERSION.txt README.txt LICENSE.txt \
		*.c *.h libmp4tag.h.in config.h.in libmp4tag.pc.in \
		man wiki \
		$(SRCDIR)
	@touch $(SRCFLAG)

# internal use
.PHONY: sourcetar
sourcetar: $(SRCFLAG)
	TARGZ=libmp4tag-src-$${LIBMP4TAG_VERSION}.tar.gz; \
	test -f $${TARGZ} && rm -f $${TARGZ}; \
	tar -c -z -f $${TARGZ} $(SRCDIR)

# internal use
.PHONY: sourcezip
sourcezip: $(SRCFLAG)
	ZIPF=libmp4tag-src-$${LIBMP4TAG_VERSION}.zip; \
	test -f $${ZIPF} && rm -f $${ZIPF}; \
	zip -q -o -r -9 $${ZIPF} $(SRCDIR)

# development tarball
.PHONY: devtar
devtar:
	@. ./VERSION.txt ; \
	SRCDIR=libmp4tag-$${LIBMP4TAG_VERSION} ; \
	TARGZ=libmp4tag-dev-$${LIBMP4TAG_VERSION}.tar.gz ; \
	$(MAKE) SRCDIR=$${SRCDIR} TARGZ=$${TARGZ} devtartgt

.PHONY: devtartgt
devtartgt:
	$(MAKE) tclean
	-test -d $(SRCDIR) && rm -rf $(SRCDIR)
	mkdir $(SRCDIR)
	cp -pfr \
		CMakeLists.txt Makefile \
		VERSION.txt \
		*.c *.h libmp4tag.h.in config.h.in libmp4tag.pc.in \
		samples tests test-files \
		man \
		$(SRCDIR)
	-test -f $(TARGZ) && rm -f $(TARGZ)
	tar -c -z -f $(TARGZ) $(SRCDIR)
	chmod -R a+w $(SRCDIR)
	rm -rf $(SRCDIR)

# cleaning

.PHONY: distclean
distclean:
	@-$(MAKE) tclean
	@-rm -rf $(BUILDDIR) $(LOCTMP) x
	@-rm -f libmp4tag-src-[0-9]*[0-9].[tz]*

.PHONY: clean
clean:
	@-$(MAKE) tclean
	@-test -d $(BUILDDIR) && cmake --build $(BUILDDIR) --target clean

.PHONY: tclean
tclean:
	@-rm -f w ww www core
	@-rm -f test-*.txt test-xx.m4a
	@-rm -f *~ */*~ *.orig */*.orig
	@-rm -f asan.*
