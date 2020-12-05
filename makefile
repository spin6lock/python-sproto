all: pysproto test
.PHONY : all

pysproto: python_sproto.c
	python3 setup.py build --build-lib .

test: pysproto
	python3 test.py
	python3 test_wild_pointer.py
	python3 test_mem.py

clean:
	rm -f *.so
	rm -rf python_sproto.egg-info
	rm -rf build
	rm -rf dist
	rm -f core
