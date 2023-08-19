libmp4tag

Contents:
  About
  Notes
  Using the mp4tagcli executable

About

  libmp4tag is a library to read and write MP4 tags.

  The goal is to have a library where all tags can be accessed and
  modified and any tags, unknown tags or custom tags are never lost
  when the audio file is updated.

  A command line utility is included to display or change the tags.

Current Status:

  Alpha

  Alpha software is under development.  There is no guarantee that the
  API will be preserved or that anything works correctly.

  At this time, libmp4tag can read tags.
  Tags can be written if there is space available.

  I only work with audio files, so this software will need testing
  against video files.

  To Do:
    - updating tags: update tags when there is not enough space.

Notes:

  The 'gnre' tag is always converted to '©gen' when writing the tag data.

  Duration is in milliseconds.

Using the mp4tagcli executable:

  If a three character tag name is specified, the copyright symbol will
  automatically be prepended to the tag name.

  The duration is displayed in milliseconds.

  Displaying the duration and tags:
    mp4tagcli filename.m4a

  Displaying the duration only:
    mp4tagcli filename.m4a --duration

  Displaying a single tag:
    mp4tagcli filename.m4a --display nam
    # note that gnre will never be found, use gen.
    mp4tagcli filename.m4a --display gen

  Dump binary data:
    mp4tagcli filename.m4a --display covr --dump > picture-data

[ The following may or may not work.  At this time, writing tags to the MP4
  file works if there is enough space available. ]

  Setting a tag:
    mp4tagcli filename.m4a nam=My-Title
    mp4tagcli filename.m4a trkn='(2,5)'

  Set a tag and display the value:
    # this will display the set value afterwards.
    # this will verify that setting tag was processed.
    # to verify that the tag was actually written, the utility must be re-run.
    mp4tagcli filename.m4a nam=My-Title --display nam

  Setting multiple tags:
    mp4tagcli filename.m4a nam=My-Title gnr=Country

  Setting a custom tag:
    mp4tagcli filename.m4a -- ----:com.apple.iTunes:CONDUCTOR=Beethoven
    mp4tagcli filename.m4a -- ----:BDJ4:DANCE=Waltz
    mp4tagcli filename.m4a -- \
        '----:com.apple.iTunes:MusicBrainz Track Id'=1234

  Setting binary data:
    mp4tagcli filename.m4a --binary covr=pic.png
    mp4tagcli filename.m4a --binary -- ----:MYAPP:ALTERNATE=filename.dat

  Deleting a tag:
    mp4tagcli filename.m4a nam=

  Deleting all tags:
    mp4tagcli filename.m4a --clean
