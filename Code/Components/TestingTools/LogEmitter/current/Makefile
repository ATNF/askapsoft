RBUILD_FLAGS=-SMTR

# Uncomment for verbose rbuild output
RBUILD_FLAGS+= -v

.PHONY: install
install:
	rbuild $(RBUILD_FLAGS) -t install

.PHONY: ftest
ftest: install
	rbuild $(RBUILD_FLAGS) -t functests

.PHONY: test
test:
	rbuild $(RBUILD_FLAGS) -t test

.PHONY: clean
clean:
	rbuild $(RBUILD_FLAGS) -t clean
	find . -type f -name '*.pyc' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	rm -f ./doxy.conf.tmp ./options.cache ./askap_logemitter.*

.PHONY: all
all: install

.PHONY: rebuild
rebuild: clean
	rbuild $(RBUILD_FLAGS) -R -t install
