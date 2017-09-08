RBUILD_FLAGS=-na

# Uncomment for verbose rbuild output
#RBUILD_FLAGS+= -v

.PHONY: install
install:
	rbuild $(RBUILD_FLAGS) -t install

.PHONY: ftest
ftest:
	rbuild $(RBUILD_FLAGS) -R -t functests

.PHONY: test
test:
	rbuild $(RBUILD_FLAGS) -R -t test

.PHONY: clean
clean:
	rbuild $(RBUILD_FLAGS) -t clean
	find . -type f -name '*-results.xml' -exec rm -f {} +
	find . -type f -name '*.pyc' -exec rm -f {} +
	find functests/ -type f -name '*.stderr' -exec rm -f {} +
	find functests/ -type f -name '*.stdout' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	rm -rf functests/data/
	rm -f ./doxy.conf.tmp ./options.cache ./askap_skymodel.* ./*.ice
	find . -type f -name '*.dbtmp' -exec rm -f {} +

.PHONY: all
all: install test ftest

.PHONY: rebuild
rebuild: clean
	rbuild $(RBUILD_FLAGS) -R -t install

.PHONY: generate
generate:
	cd ./schema_definitions && python3 generate.py
	python3 ./run_odb_compiler.py

.PHONY: generate_ice
generate_ice: generate
	mv *.ice ../../../../Interfaces/slice/current/
