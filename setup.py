from distutils.core import setup, Extension

pysproto_module = Extension('pysproto',
        sources = ["python_sproto.c", "sproto.c"])

setup (name = "python_sproto",
        version = '0.1',
        description = "python binding to sproto, a serialize library",
        ext_modules = [pysproto_module])
