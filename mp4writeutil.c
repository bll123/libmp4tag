/*
 * Copyright 2023 Brad Lanam Pleasant Hill CA
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "libmp4tag.h"
#include "mp4tagint.h"
#include "mp4tagbe.h"

void
mp4tag_update_parent_lengths (libmp4tag_t *libmp4tag, FILE *ofh, int32_t delta)
{
  int     idx;
  int     rc;

  idx = libmp4tag->parentidx;

  if (idx >= 0 && libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
    fprintf (stdout, "  update-parent-lengths\n");
    fprintf (stdout, "    delta: %d\n", delta);
  }

  while (idx >= 0) {
    uint32_t    t32;

    rc = fseek (ofh, libmp4tag->base_offsets [idx], SEEK_SET);
    if (rc != 0) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_SEEK_ERROR;
      return;
    }

    t32 = libmp4tag->base_lengths [idx] + delta;
    if (libmp4tag->dbgflags & MP4TAG_DBG_WRITE) {
      fprintf (stdout, "    update-parent: idx: %d %s len: %d / %d\n", idx, libmp4tag->base_name [idx], libmp4tag->base_lengths [idx], t32);
    }
    t32 = htobe32 (t32);
    if (fwrite (&t32, sizeof (uint32_t), 1, ofh) != 1) {
      libmp4tag->mp4error = MP4TAG_ERR_FILE_WRITE_ERROR;
    }
    --idx;
  }
}

void
mp4tag_auto_fix (libmp4tag_t *libmp4tag, uint64_t delta, int level)
{
  int     savedidx;

  savedidx = libmp4tag->parentidx;
  libmp4tag->parentidx = level - 1;
  mp4tag_update_parent_lengths (libmp4tag, libmp4tag->fh, - delta);
  /* update base lengths to match */
  for (int i = libmp4tag->parentidx; i >= 0; --i) {
    libmp4tag->base_lengths [i] -= delta;
  }
  libmp4tag->parentidx = savedidx;
}
