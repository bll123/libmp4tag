#
# Copyright 2021-2023 Brad Lanam Pleasant Hill CA
#
cmake_minimum_required (VERSION 3.18)

# avoid msys2/windows issue
set (CMAKE_C_COMPILER_WORKS 1)
set (CMAKE_CXX_COMPILER_WORKS 1)

project (LIBMP4TAG VERSION ${LIBMP4TAG_BUILD_VERS})

set (default_build_type "Release")

set (CMAKE_SHARED_LIBRARY_PREFIX "")
set (CMAKE_STATIC_LIBRARY_PREFIX "")
set (CMAKE_INSTALL_PREFIX "")

include_directories (
  "${CMAKE_BINARY_DIR}"
)

# compile options

add_compile_options (-fPIC)

add_compile_options (-Wall)
add_compile_options (-Wextra)
add_compile_options (-Wno-unused-parameter)
add_compile_options (-Wno-unknown-pragmas)
add_compile_options (-Wno-float-equal)
add_compile_options (-Wdeclaration-after-statement)
add_compile_options (-Wmissing-prototypes)
add_compile_options (-Wformat)
add_compile_options (-Wformat-security)
add_compile_options (-Werror=format-security)
add_compile_options (-Wdeprecated-declarations)

#### compiler-specific compile options

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
  add_compile_options (-Wmaybe-uninitialized)
  add_compile_options (-Wno-unused-but-set-variable)
  add_compile_options (-Wno-stringop-overflow)
  add_compile_options (-Wno-stringop-truncation)
  add_compile_options (-Wno-format-truncation)
endif()
if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
  add_compile_options (-Wno-poison-system-directories)
  add_compile_options (-Wno-shift-sign-overflow)
  add_compile_options (-Wno-pragma-pack)
  add_compile_options (-Wno-ignored-attributes)
  if (APPLE)
    add_compile_options (-Wno-reserved-macro-identifier)
  endif()
  add_compile_options (-Wno-reserved-id-macro)
  add_compile_options (-Wno-implicit-int-conversion)
  add_compile_options (-Wno-switch-enum)
  add_compile_options (-Wno-gnu-zero-variadic-macro-arguments)
  add_compile_options (-Wno-documentation-deprecated-sync)
  add_compile_options (-Wno-documentation-unknown-command)
  add_compile_options (-Wno-documentation)
endif()

#### build compile options

if (LIBMP4TAG_BUILD STREQUAL "Release")
  add_compile_options (-O2)
endif()

if (LIBMP4TAG_BUILD STREQUAL "Debug")
  add_compile_options (-O0)
endif()

add_compile_options (-g)
add_link_options (-g)
if (NOT WIN32)
  add_link_options (-rdynamic)
endif()

# more compile options: fortification/address sanitizer

set (LIBMP4TAG_FORTIFY T)

# address sanitizer
if (LIBMP4TAG_BUILD STREQUAL "SanitizeAddress")
  message ("Sanitize Address Build")
  set (LIBMP4TAG_FORTIFY F)
  add_compile_options (-O0)
  add_compile_options (-ggdb)
  add_link_options (-g)
  add_compile_options (-fsanitize=address)
  add_link_options (-fsanitize=address)
  add_compile_options (-fsanitize-address-use-after-scope)
  add_link_options (-fsanitize-address-use-after-scope)
  add_compile_options (-fsanitize-recover=address)
  add_link_options (-fsanitize-recover=address)
  add_compile_options (-fno-omit-frame-pointer)
  add_compile_options (-fno-common)
  add_compile_options (-fno-inline)
  if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
    add_link_options (-lrt)
  endif()
endif()

if (LIBMP4TAG_FORTIFY STREQUAL T)
  # hardening
  add_compile_options (-fstack-protector-strong)
  add_compile_options (-fstack-protector-all)
  add_compile_options (-fstack-protector-strong)
  add_compile_options (-fstack-protector-all)
  add_compile_options (-D_FORTIFY_SOURCE=2)
else()
  if (CMAKE_C_COMPILER_ID STREQUAL "Clang")
    add_compile_options (-Wno-macro-redefined)
  endif()
  add_compile_options (-U_FORTIFY_SOURCE)
  add_compile_options (-D_FORTIFY_SOURCE=0)
endif()

#### system specific compile options

if (NOT WIN32)
  SET (CMAKE_INSTALL_RPATH "\${ORIGIN}")
  if (APPLE)
    # 10.14 = Mojave, 10.15 = Catalina
    # 11 = Big Sur, 12 = Monterey, 13 = Ventura, 14 = Sonoma
    add_compile_options (-mmacosx-version-min=10.14)
    add_link_options (-mmacosx-version-min=10.14)
    # The following does not work.
    # SET (CMAKE_INSTALL_RPATH "@executable_path")
  endif()

  add_compile_options (-DMG_ARCH=MG_ARCH_UNIX)
else()
  add_compile_options (-DMG_ARCH=MG_ARCH_WIN32)
  add_link_options (-static-libgcc)
  add_link_options (-static-libstdc++)
endif()

# configuration

include (CheckIncludeFile)

check_include_file (endian.h _hdr_endian)

configure_file (config.h.in config.h)

# libraries

add_library (libmp4tag SHARED
  libmp4tag.c
  mp4tagparse.c
  mp4tagutil.c
  tagdef.c
)

add_executable (mp4tagcli
  mp4tagcli.c
)
target_link_libraries (mp4tagcli
  libmp4tag
)
