libmp4tag

Contents:
  Release Notes
  About
  Notes
  Build Requirements
  Building
  Using the mp4tagcli executable

Release Notes

  2.0.0
    mp4tagpub_t has changed.

  1.3.0
    Added support for streams.  This functionality is in beta status.

  1.2.9
    The build/install process has been brought into line with cmake's
    methods.  See the Building section.

About

  An MP4 tagging library where all tags can be accessed and modified and
  any tags, unknown tags or custom tags are never lost when the audio
  file is updated. A list of known tags is only used when new tags are
  added.

  A command line utility is included to display or change the tags.

  Wiki: https://sourceforge.net/p/libmp4tag/wiki/Home/

Current Status:

  Production

  The API is stable, and all functionality is working.

  To Do:
    - Other languages ?
      I do not have any good samples of this.

Notes:

  The 'gnre' tag is always converted to '©gen' when writing the tag
  data, and '©gen' is used internally.

  Duration is in milliseconds.

Build Requirements:

  make
  cmake
  C compiler
  libmp4tag does not use any external libraries.

Building:
  make PREFIX=target-install-dir
  make install
  # for staging
  make DESTDIR=stage-dir install

Using the mp4tagcli executable:

  If a three character tag name is specified, the copyright symbol will
  automatically be prepended to the tag name.

  The duration is displayed in milliseconds.

  Usage:
      mp4tagcli --version
      mp4tagcli \
          [--debug <value>] \
          --copyfrom in-filename --copyto out-filename
      mp4tagcli <filename> [--debug <value>] --clean
      mp4tagcli <filename> [--debug <value>] --duration
      mp4tagcli <filename> \
          [--debug <value>] \
          [--binary] [<tag>={|<value>|<filename>}] ...] \
          [--display <tag> [--dump <filename>]]

      --dump is only relevant for binary data.
      --binary is only needed when adding an unknown binary data tag
      tag=<filename> is only used for binary data.

  Displaying the duration and tags:
    mp4tagcli filename.m4a

  Displaying the duration only:
    mp4tagcli filename.m4a --duration

  Displaying a single tag:
    mp4tagcli filename.m4a --display nam
    # note that gnre will never be found, use gen.
    mp4tagcli filename.m4a --display gen

  Dump binary data:
    mp4tagcli filename.m4a --display covr --dump pic.png

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
        '----:com.apple.iTunes:MusicBrainz Track Id=1234'

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
    # the indexes are only useful if there is already an existing cover
    # image at that index. e.g. setting covr:1 in a file with no cover
    # images will place the cover at index 0.

  Arrays:
    mp4tagcli filename.m4a wrt=Composer-1
    mp4tagcli filename.m4a wrt:1=Composer-2
    mp4tagcli filename.m4a wrt:2=Composer-3

  Deleting a tag:
    mp4tagcli filename.m4a nam=

  Deleting all tags:
    mp4tagcli filename.m4a --clean

  Copy all tags:
    # this was implemented to test the preserve/restore functionality
    # of the library.
    mp4tagcli --copyfrom aaa.m4a --copyto bbb.m4a
