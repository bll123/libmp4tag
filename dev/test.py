from mutagen.mp4 import MP4,MP4FreeForm,AtomDataType
#audiof = MP4('pm.m4a')
#if audiof.tags is None:
#  audiof.add_tags()
#  audiof.save()
audio = MP4('tt.m4a')
audio['tvsn'] = [ 156 ]
audio.save()
