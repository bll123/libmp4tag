libmp4tag

  Please make a backup before using this software.  It is new, and
  hasn't yet been tested thoroughly.

Contents:
  About
  Notes
  Using the mp4tagcli executable

About

  An MP4 tagging library where all tags can be accessed and modified and
  any tags, unknown tags or custom tags are never lost when the audio
  file is updated. A list of known tags is only used when new tags are
  added.

  A command line utility is included to display or change the tags.

Current Status:

  Beta

  Beta software is under development.  The API should be (mostly)
  stable, and most functionality should be working.  There may still
  be some bugs present.

  To Do:
    - better documentation

Notes:

  The 'gnre' tag is always converted to '©gen' when writing the tag
  data, and '©gen' is used internally.

  Duration is in milliseconds.

Using the mp4tagcli executable:

  If a three character tag name is specified, the copyright symbol will
  automatically be prepended to the tag name.

  The duration is displayed in milliseconds.

  Usage:
      mp4tagcli <filename> [--version] [--debug <value>] \
          [--duration] [--clean] [--display <tag> [--dump=<filename>]]
          [--binary] [<tag>={|<value>|<filename>}] ...]

      --dump is only relevant for binary data.
      --binary is only needed when adding an unknown binary data tag
      tag=<filename> is only used for binary data.

  Displaying known tags:
    mp4tagcli --list-tags

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

  Setting a tag:
    mp4tagcli filename.m4a nam=My-Title
    mp4tagcli filename.m4a trkn=2/5

  Set a tag and display the value:
    # this will display the set value afterwards.
    # this will verify that setting the tag was processed.
    # to verify that the tag was actually written, the utility must be re-run.
    mp4tagcli filename.m4a nam=My-Title --display nam

  Setting multiple tags:
    mp4tagcli filename.m4a nam=My-Title gnr=Country

  Setting a custom tag:
    mp4tagcli filename.m4a -- ----:com.apple.iTunes:CONDUCTOR=Beethoven
    mp4tagcli filename.m4a -- ----:BDJ4:DANCE=Waltz
    mp4tagcli filename.m4a -- \
        '----:com.apple.iTunes:MusicBrainz Track Id'=1234

  Binary Data:
    # existing tags and cover images do not need the --binary argument.
    # specify a filename holding the binary data as the argument
    mp4tagcli filename.m4a covr=pic.png
    # setting a new tag that is unknown and needs to be binary data.
    mp4tagcli filename.m4a --binary -- ----:MYAPP:ALTERNATE=filename.dat

  Cover Images and Cover Names:
    mp4tagcli filename.m4a covr=picA.png
    mp4tagcli filename.m4a covr:1=picB.png
    mp4tagcli filename.m4a covr:1=picB.png covr:1:name=Back
    mp4tagcli filename.m4a covr:0:name=Front

  Deleting a tag:
    mp4tagcli filename.m4a nam=

  Deleting all tags:
    mp4tagcli filename.m4a --clean

  Copy all tags:
    # this was implemented to test the preserve/restore functionality
    # of the library.
    mp4tagcli --copyfrom aaa.m4a --copyto bbb.m4a
