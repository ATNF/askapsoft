RBUILD_FLAGS=-na

# Uncomment for verbose rbuild output
#RBUILD_FLAGS+= -v

# C++11 support
RBUILD_FLAGS+=-p "cpp11=1" -p "squash=1"

.PHONY: install
install:
	rbuild $(RBUILD_FLAGS) -t install

.PHONY: ftest
ftest:
	#rbuild $(RBUILD_FLAGS) -R -t functests
	./functests/run.sh

.PHONY: test
test:
	rbuild $(RBUILD_FLAGS) -R -t test

.PHONY: clean
clean:
	rbuild $(RBUILD_FLAGS) -R -t clean
	# Build artifacts not hit by rbuild -t clean
	rm -f ./doxy.conf.tmp ./options.cache ./askap_pipelinetasks.cc
	rm -f ./doxy.conf.tmp ./options.cache ./askap_pipelinetasks.cc
	rm -rf functests/test_data_accessors/ice_data
	find . -type f -name '*.log' -exec rm -f {} +
	find . -type f -name '*-results.xml' -exec rm -f {} +
	#find . -type f -name '*.dbtmp' -exec rm -f {} +

.PHONY: all
all: install test ftest
