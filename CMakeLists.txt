#
# Copyright 2023-2025 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.13 FATAL_ERROR)

# avoid msys2/windows issue
set (CMAKE_C_COMPILER_WORKS 1)
set (CMAKE_CXX_COMPILER_WORKS 1)

###
# check to make sure cmake-install-prefix is set
if (NOT DEFINED CMAKE_INSTALL_PREFIX OR CMAKE_INSTALL_PREFIX STREQUAL "")
  message (FATAL_ERROR "CMAKE_INSTALL_PREFIX is not set")
endif()

###
# load the LIBMP4TAG_* variables from VERSION.txt

file (STRINGS VERSION.txt tlibmp4tagvars)
foreach (tvar IN LISTS tlibmp4tagvars)
  # the export lines are not needed
  if (tvar MATCHES "=")
    string (REGEX MATCH "^[^=]*" tnm ${tvar})
    string (REGEX MATCH "=.*" tvala ${tvar})
    string (REPLACE "=" "" tval ${tvala})
    set (${tnm} ${tval})
  endif()
endforeach()

project (LIBMP4TAG
  VERSION ${LIBMP4TAG_VERSION}
  DESCRIPTION "MP4 tagging library"
  HOMEPAGE_URL "https://libmp4tag.sourceforge.io"
  LANGUAGES C
)

# It is possible that the so-version may not match the main version
# in some future.  This would need to be changed.
string (REGEX REPLACE "\\..*" "" LIBMP4TAG_SOVERSION ${LIBMP4TAG_VERSION})

option (BUILD_SHARED_LIBS "Build dynamic library" ON)

set (default_build_type "Release")

include (GNUInstallDirs)

include (CheckCCompilerFlag)
include (CheckLinkerFlag)
include (CheckFunctionExists)
include (CheckSymbolExists)
include (CheckStructHasMember)

set (LIBMP4TAG_LIBNAME libmp4tag)
if (WIN32)
  # msys2 loves to put the lib prefix in front
  # difficult to work around
  set (LIBMP4TAG_LIBNAME mp4tag)
endif()

include_directories (
  "${CMAKE_BINARY_DIR}"
)

SET (CMAKE_SKIP_BUILD_RPATH FALSE)
SET (CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
if (DEFINED ENV{CMAKE_SKIP_RPATH} AND NOT ENV{CMAKE_SKIP_RPATH} STREQUAL "")
  set (CMAKE_SKIP_RPATH "$ENV{CMAKE_SKIP_RPATH}")
endif()

#### compile options

macro (checkAddCompileFlag flag)
  string (REPLACE "-" "_" tflag ${flag})
  string (REPLACE "=" "_" tflag ${tflag})
  check_c_compiler_flag (${flag} cfchk${tflag})
  if (cfchk${tflag})
    add_compile_options (${flag})
  endif()
endmacro()

macro (checkAddCompileNegFlag flag)
  # cmake caches every damned variable...
  # need a different variable for every test.
  string (REPLACE "-" "_" tflag ${flag})
  string (REPLACE "=" "_" tflag ${tflag})
  string (REPLACE "no-" "" chkflag ${flag})
  check_c_compiler_flag (${chkflag} cfchk${tflag})
  if (cfchk${tflag})
    add_compile_options (${flag})
  endif()
endmacro()

macro (checkAddLinkFlag flag)
  string (REPLACE "-" "_" tflag ${flag})
  string (REPLACE "=" "_" tflag ${tflag})
  check_linker_flag ("C" ${flag} lfchk${tflag})
  if (lfchk${tflag})
    add_link_options (${flag})
  endif()
endmacro()

checkAddCompileFlag ("-fPIC")
checkAddLinkFlag ("-fPIC")

checkAddCompileFlag ("-Wall")
checkAddCompileFlag ("-Wextra")
checkAddCompileNegFlag ("-Wno-unused-parameter")
checkAddCompileNegFlag ("-Wno-unknown-pragmas")
checkAddCompileNegFlag ("-Wno-float-equal")
checkAddCompileFlag ("-Wdeclaration-after-statement")
checkAddCompileFlag ("-Wmissing-prototypes")
checkAddCompileFlag ("-Wformat")
checkAddCompileFlag ("-Wformat-security")
checkAddCompileFlag ("-Werror=format-security")
checkAddCompileFlag ("-Wdeprecated-declarations")

#### compiler-specific compile options

checkAddCompileFlag ("-Wmaybe-uninitialized")
checkAddCompileNegFlag ("-Wno-unused-but-set-variable")
checkAddCompileNegFlag ("-Wno-stringop-overflow")
checkAddCompileNegFlag ("-Wno-stringop-truncation")
checkAddCompileNegFlag ("-Wno-format-truncation")
checkAddCompileNegFlag ("-Wno-poison-system-directories")
checkAddCompileNegFlag ("-Wno-shift-sign-overflow")
checkAddCompileNegFlag ("-Wno-pragma-pack")
checkAddCompileNegFlag ("-Wno-ignored-attributes")
checkAddCompileNegFlag ("-Wno-reserved-macro-identifier")
checkAddCompileNegFlag ("-Wno-reserved-id-macro")
checkAddCompileNegFlag ("-Wno-implicit-int-conversion")
checkAddCompileNegFlag ("-Wno-switch-enum")
checkAddCompileNegFlag ("-Wno-gnu-zero-variadic-macro-arguments")
checkAddCompileNegFlag ("-Wno-documentation-deprecated-sync")
checkAddCompileNegFlag ("-Wno-documentation-unknown-command")
checkAddCompileNegFlag ("-Wno-documentation")

#### build compile options

if (NOT DEFINED LIBMP4TAG_BUILD OR LIBMP4TAG_BUILD STREQUAL "")
  set (LIBMP4TAG_BUILD "Release")
endif()

if (LIBMP4TAG_BUILD STREQUAL "Release")
  add_compile_options (-O2)
endif()

if (LIBMP4TAG_BUILD STREQUAL "Debug")
  add_compile_options (-O0)
endif()

add_compile_options (-g)
add_link_options (-g)
if (NOT WIN32)
  checkAddLinkFlag ("-rdynamic")
endif()

#### more compile options: fortification/address sanitizer

set (LIBMP4TAG_FORTIFY T)
if (DEFINED ENV{LIBMP4TAG_FORTIFY} AND NOT ENV{LIBMP4TAG_FORTIFY} STREQUAL "")
  set (LIBMP4TAG_FORTIFY $ENV{LIBMP4TAG_FORTIFY})
endif()

# address sanitizer
if (LIBMP4TAG_BUILD STREQUAL "SanitizeAddress")
  set (LIBMP4TAG_FORTIFY F)
  add_compile_options (-O0)
  checkAddCompileFlag ("-ggdb")
  add_link_options (-g)
  checkAddCompileFlag ("-fsanitize=address")
  checkAddLinkFlag ("-fsanitize=address")
  checkAddCompileFlag ("-fsanitize-address-use-after-scope")
  checkAddLinkFlag ("-fsanitize-address-use-after-scope")
  checkAddCompileFlag ("-fsanitize-recover=address")
  checkAddLinkFlag ("-fsanitize-recover=address")
  checkAddCompileFlag ("-fno-omit-frame-pointer")
  checkAddCompileFlag ("-fno-common")
  checkAddCompileFlag ("-fno-inline")
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    checkAddLinkFlag ("-lrt")
  endif()
endif()

if (LIBMP4TAG_FORTIFY STREQUAL Y AND NOT CMAKE_HOST_SOLARIS)
  # hardening
  checkAddCompileFlag ("-fstack-protector-strong")
  checkAddCompileFlag ("-fstack-protector-all")
  checkAddCompileFlag ("-D_FORTIFY_SOURCE=2")
else()
  checkAddCompileNegFlag ("-Wno-macro-redefined")
  add_compile_options (-U_FORTIFY_SOURCE)
  add_compile_options (-D_FORTIFY_SOURCE=0)
endif()

#### system specific compile options

if (NOT WIN32)
  if (NOT APPLE)
    SET (CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR} "\${ORIGIN}")
  endif()
  if (APPLE)
    SET (CMAKE_INSTALL_RPATH ${CMAKE_INSTALL_FULL_LIBDIR} "@loader_path")
    # 10.6 = Snow Leopard, 10.7 = Lion
    # 10.8 = Mountain Lion, 10.9 = Mavericks
    # 10.10 = Yosemite, 10.11 = El Capitan
    # 10.12 = Sierra, 10.13 = High Sierra
    # 10.14 = Mojave, 10.15 = Catalina
    # 11 = Big Sur, 12 = Monterey, 13 = Ventura, 14 = Sonoma
    # 15 = Sequoia, 26 = Tahoe
    set (CMAKE_OSX_DEPLOYMENT_TARGET 10.13)
    # universal binary
    set (CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
  endif()
else()
  checkAddLinkFlag ("-static-libgcc")
  checkAddLinkFlag ("-static-libstdc++")
endif()

#### configuration

# windows functions
check_function_exists (GetFileTime _lib_GetFileTime)
check_function_exists (Sleep _lib_Sleep)
check_function_exists (CommandLineToArgvW _lib_CommandLinetoArgvW)
check_function_exists (_wrename _lib__wrename)
check_function_exists (_wstat64 _lib__wstat64)
check_function_exists (_wunlink _lib__wunlink)

check_symbol_exists (nanosleep time.h _lib_nanosleep)
check_symbol_exists (setrlimit sys/resource.h _lib_setrlimit)

check_struct_has_member ("struct stat"
    st_atim sys/stat.h _mem_struct_stat_st_atim)
check_struct_has_member ("struct stat"
    st_atimespec sys/stat.h _mem_struct_stat_st_atimespec)

configure_file (config.h.in config.h)
configure_file (libmp4tag.h.in libmp4tag.h)

#### libraries

add_library (${LIBMP4TAG_LIBNAME}
  libmp4tag.c
  mp4tagfileop.c
  mp4tagparse.c
  mp4tagwrite.c
  mp4tagutil.c
  mp4writeutil.c
  mp4const.c
)
set_target_properties (${LIBMP4TAG_LIBNAME} PROPERTIES PREFIX "")
if (WIN32)
  set_target_properties (${LIBMP4TAG_LIBNAME} PROPERTIES PREFIX "lib")
endif()
if (WIN32)
  target_link_libraries (${LIBMP4TAG_LIBNAME} PUBLIC ws2_32)
endif()

# I don't know if this is needed.  windows works fine for me.
if (WIN32)
  set_target_properties (${LIBMP4TAG_LIBNAME} PROPERTIES WINDOWS_EXPORT_ALL_SYMBOLS ON)
endif()
set_target_properties (${LIBMP4TAG_LIBNAME} PROPERTIES
  VERSION ${CMAKE_PROJECT_VERSION}
  SOVERSION ${LIBMP4TAG_SOVERSION}
)

#### executables

add_executable (mp4tagcli
  mp4tagcli.c
)
target_link_libraries (mp4tagcli PRIVATE
  ${LIBMP4TAG_LIBNAME}
)
if (WIN32)
  target_link_libraries (mp4tagcli PUBLIC ws2_32)
endif()

# libmp4tag.pc

configure_file (${CMAKE_SOURCE_DIR}/libmp4tag.pc.in libmp4tag.pc @ONLY)

# compressed manual pages

#### install

install (TARGETS ${LIBMP4TAG_LIBNAME}
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  # windows seems to need this
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install (TARGETS mp4tagcli
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)

install (FILES ${CMAKE_BINARY_DIR}/libmp4tag.h
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

install (FILES man/mp4tagcli.1
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man1"
)
install (FILES man/libmp4tag.3
  DESTINATION "${CMAKE_INSTALL_DATADIR}/man/man3"
)

install (FILES
  ${CMAKE_BINARY_DIR}/libmp4tag.pc
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)
