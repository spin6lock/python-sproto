from distutils.core import setup, Extension

mymodule = Extension('example',
        sources = ["example.c", "sproto.c"])

setup (name = "example",
        version = '1.0',
        description = "add example",
        ext_modules = [mymodule])
