libmp4tag

  libmp4tag is a library to read and write MP4 tags.

  The goal is to have a library where all tags can be accessed and
  modified and any tags or custom tags are never lost when the audio
  file is updated.

  A command line utility is included to display or change the tags.

Current Status:

  Alpha

  Alpha software is under development.  There is no guarantee that the
  API will be preserved or that anything works correctly.

  At this time, libmp4tag can read tags.  Tags cannot be written.

  I only work with audio files, so this software will need testing
  against video files.

  To Do:
    - updating tags: update tags in-place when there is available space.
    - updating tags: update tags when there is not enough space.
