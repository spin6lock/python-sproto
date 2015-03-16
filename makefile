all:example.c
	python setup.py build
	mv build/lib.linux-x86_64-2.7/example.so .
	python test.py
