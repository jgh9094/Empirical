doc: build-doxygen-xml
	cd doc && make html

build-doxygen-xml:
	mkdir -p build/doxydoc/source
	./third-party/doxygen/build/bin/doxygen Doxyfile

diff-cover: coverage
	git diff origin/master > scripts/diff
	cd scripts && python3 vigilance.py -i ignore_list diff

travis: 
	make test

test:
	cd tests && make test

coverage:
	cd tests && make coverage

coverage-html:
	cd tests && make coverage-html

install-dependencies:
	git submodule init
	git submodule update
	cd third-party && make

install-testing-dependencies:
	cd third-party && make install-testing-dependencies

clean:
	rm -rf build/*
	cd tests && make clean
clean-dep:
	cd third-party && make clean
