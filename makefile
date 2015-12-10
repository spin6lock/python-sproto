all: pysproto test
.PHONY : all

pysproto: python_sproto.c
	python setup.py build --build-lib .

test: pysproto
	python test.py
	python test_wild_pointer.py

clean:
	rm *.so
	rm -rf python_sproto.egg-info
	rm -rf build
	rm -rf dist
