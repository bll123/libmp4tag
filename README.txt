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

Using the mp4tagcli executable:

  If a three character tag name is specified, the copyright symbol will
  automatically be prepended to the tag name.

  The duration is displayed in milliseconds.

  Displaying the duration and tags:
    ./mp4tagcli filename.m4a
  Displaying the duration only:
    ./mp4tagcli filename.m4a --duration
  Displaying a single tag:
    ./mp4tagcli filename.m4a --display nam
    # note that gnre will never be found, use gnr.
    ./mp4tagcli filename.m4a --display gnr
  Dump binary data:
    ./mp4tagcli filename.m4a --display covr --dump > picture-data

  The following are not yet implemented:

  Setting a tag:
    ./mp4tagcli filename.m4a nam=My-Title
  Setting multiple tags:
    ./mp4tagcli filename.m4a nam=My-Title gnr=Country
  Setting a custom tag:
    ./mp4tagcli filename.m4a -- ----:com.apple.iTunes:CONDUCTOR=Beethoven
    ./mp4tagcli filename.m4a -- ----:BDJ4:DANCE=Waltz
    ./mp4tagcli filename.m4a -- \
        '----:com.apple.iTunes:MusicBrainz Track Id'=1234
  Setting binary data:
    ./mp4tagcli filename.m4a covr=pic.png
    ./mp4tagcli filename.m4a --binary -- ----:MYAPP:ALTERNATE=filename.dat
  Deleting all tags:
    ./mp4tagcli filename.m4a --clean
