RBUILD_FLAGS=-SMTR
CPMAN_FILES=./cpmanager.jar ./cpmanager.sh

# Uncomment for verbose rbuild output
#RBUILD_FLAGS+= -v

.PHONY: all
all: $(CPMAN_FILES) test ftest

.PHONY: ftest
ftest: $(CPMAN_FILES)
	rbuild $(RBUILD_FLAGS) -t functests

.PHONY: test
test: $(CPMAN_FILES)  
	rbuild $(RBUILD_FLAGS) -t test

.PHONY: clean
clean:
	find . -type f -name '*junit-results.xml' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	find . -type f -name '*.pyc' -exec rm -f {} +
	find . -type d -name '*.ms' -exec rm -rf {} +
	rm -rf functests/data/ install/ build/askap/
	rm -f ./cpmanager*

.PHONY: install
install: $(CPMAN_FILES)

$(CPMAN_FILES):
	rbuild $(RBUILD_FLAGS) -t install
