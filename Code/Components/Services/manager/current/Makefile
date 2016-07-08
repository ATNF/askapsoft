RBUILD_FLAGS=-SMTR
CPMAN_FILES=./cpmanager ./cpmanager.jar ./cpmanager.sh

# Uncomment for verbose rbuild output
#RBUILD_FLAGS+= -v

all: $(CPMAN_FILES) test ftest

ftest: $(CPMAN_FILES)
	rbuild $(RBUILD_FLAGS) -t functests

test: $(CPMAN_FILES)  
	rbuild $(RBUILD_FLAGS) -t test

clean:
	find . -type f -name '*junit-results.xml' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	find . -type f -name '*.pyc' -exec rm -f {} +
	find . -type d -name '*.ms' -exec rm -rf {} +
	rm -rf functests/data/
	rm -rf install/
	rm -f ./cpmanager*

$(CPMAN_FILES):
	rbuild $(RBUILD_FLAGS) -t install
