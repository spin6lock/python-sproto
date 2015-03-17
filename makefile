all:python_sproto.c
	python setup.py build
	mv build/lib.linux-x86_64-2.7/pysproto.so .
	python test.py
