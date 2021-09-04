all: pysproto test
.PHONY : all

pysproto: src/pysproto/python_sproto.c
	python3 setup.py build --build-lib build

test: pysproto
	cp build/*.so test/
	cd test && python3 test.py && python3 test_wild_pointer.py && python3 test_mem.py

clean:
	rm -f *.so
	rm -rf python_sproto.egg-info
	rm -rf build
	rm -rf dist
	rm -f core

sdist:
	rm -f dist/*
	python3 setup.py sdist

publish:
	twine upload dist/*.tar.gz
