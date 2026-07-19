import py7zr
import shutil

@function.supports_root_dir(True)
def pack_demo():
    pass

shutil.register_archive_format('demopak', pack_demo)
