import setuptools
from distutils.core import setup, Extension

pysproto_module = Extension('pysproto',
        sources = ["src/pysproto/python_sproto.c", "src/pysproto/sproto.c"])

with open("README.md", "r", encoding="utf-8") as fh:
        long_description = fh.read()

setup(name = "pysproto",
        version = '0.1.4',
        description = "python binding to sproto, a serialize library",
        long_description = "Documentation and bug report: http://github.com/spin6lock/python-sproto",
        author = "spin6lock",
        author_email = "UNKNOWN",
        package_dir = {"": "src"},
        packages = setuptools.find_packages(where="src"),
        package_data = {"pysproto": ["src/pysproto/msvcint.h", "src/pysproto/sproto.h"]},
        python_requires = ">=3.6",
        license = "MIT",
        url = "http://github.com/spin6lock/python-sproto",
        keywords = ["sproto", "serialization"],
        ext_modules = [pysproto_module])
