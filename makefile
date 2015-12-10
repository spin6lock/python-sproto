all:python_sproto.c
	python setup.py build --build-lib .

test: test.py test_wild_pointer.py
	python test.py
	python test_wild_pointer.py
