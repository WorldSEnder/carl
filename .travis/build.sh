#!/usr/bin/env bash

mkdir build || return 1
cd build/ || return 1
cmake -D DEVELOPER=ON -D USE_CLN_NUMBERS=ON -D USE_GINAC=ON -D USE_COCOA=ON ../ || return 1

MAKE_PARALLEL="-j2"

if [[ ${TASK} == "coverage" ]]; then
	gem install coveralls-lcov
	cmake -D DEVELOPER=ON -D USE_CLN_NUMBERS=ON -D USE_GINAC=ON -D USE_COCOA=ON -D COVERAGE=ON ../ || return 1
	
	/usr/bin/time make ${MAKE_PARALLEL} resources || return 1
	/usr/bin/time make ${MAKE_PARALLEL} lib_carl || return 1
	/usr/bin/time make ${MAKE_PARALLEL} || return 1
	/usr/bin/time make ${MAKE_PARALLEL} coverage-collect || return 1
	
	coveralls-lcov --repo-token ${COVERALLS_TOKEN} coverage.info
elif [[ ${TASK} == "doxygen" ]]; then
	make doc || return 1
	
	git config --global user.email "gereon.kremer@cs.rwth-aachen.de"
	git config --global user.name "Travis doxygen daemon"
	
	git clone https://${GH_TOKEN}@github.com/smtrat/smtrat.github.io.git
	cd smtrat.github.io/ || return 1
	
	# Update cloned copy
	cp -r ../doc/html/* carl/ || return 1
	# Check if something has changed
	git diff --summary --exit-code && return 0
	git add carl/ || return 1
	# Commit and push
	git commit -m "Updated documentation for carl" || return 1
	git push origin master || return 1

elif [[ ${TASK} == "pycarl" ]]; then
	
	# Create a python virtual environment for local installation
	virtualenv -p python3 pycarl-env
	source pycarl-env/bin/activate
	
	/usr/bin/time make ${MAKE_PARALLEL} resources || return 1
	/usr/bin/time make ${MAKE_PARALLEL} lib_carl || return 1
	
	# Clone pycarl
	git clone https://github.com/moves-rwth/pycarl.git
	cd pycarl/ || return 1
	# Build bindings
	python setup.py build_ext -j 1 develop || return 1
	# Run tests
	py.test tests/ || return 1
	
else
	/usr/bin/time make ${MAKE_PARALLEL} resources || return 1
	/usr/bin/time make ${MAKE_PARALLEL} lib_carl || return 1
	/usr/bin/time make ${MAKE_PARALLEL} || return 1
	/usr/bin/time make -j1 CTEST_OUTPUT_ON_FAILURE=1 test || return 1
fi

cd ../
