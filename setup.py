from distutils.core import setup, Extension

pysproto_module = Extension('pysproto',
        sources = ["python_sproto.c", "sproto.c"])

setup (name = "python_sproto",
        version = '0.1',
        description = "python binding to sproto, a serialize library",
        long_description = "Documentation and bug report: http://github.com/spin6lock/python-sproto",
        author = "spin6lock",
        license = "MIT",
        url="http://github.com/spin6lock/python-sproto",
        keywords=["sproto", "serialization"],
        ext_modules = [pysproto_module])
