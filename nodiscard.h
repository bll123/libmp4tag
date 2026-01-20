/*
 * Copyright 2025-2026 Brad Lanam Pleasant Hill CA
 */
#pragma once

#include "config.h"

#if __STDC_VERSION__ < 202000
# undef NODISCARD
# define NODISCARD __attribute__ ((warn_unused_result))
#endif
#if _has_nodiscard
# undef NODISCARD
# define NODISCARD [[nodiscard]]
#endif
#if ! defined (NODISCARD)
# define NODISCARD
#endif
