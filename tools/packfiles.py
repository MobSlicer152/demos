import py7zr
import shutil

def pack_demo():
    pass

shutil.register_archive_format('demopak', pack_demo)
