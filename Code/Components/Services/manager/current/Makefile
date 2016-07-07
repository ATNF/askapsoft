RBUILD_FLAGS=-SMTR

# Uncomment for verbose rbuild output
#RBUILD_FLAGS+= -v

ftest: cpmanager
	rbuild $(RBUILD_FLAGS) -t functests

test: cpmanager
	rbuild $(RBUILD_FLAGS) -t test

all: cpmanager test ftest

clean:
	find . -type f -name '*junit-results.xml' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	find . -type f -name '*.pyc' -exec rm -f {} +
	find . -type d -name '*.ms' -exec rm -rf {} +
	rm -rf functests/data/
	rm -rf install/
	rm -f ./cpmanager*

cpmanager: cpmanager.jar cpmanager.sh

cpmanager.sh:
cpmanager.jar:
	rbuild $(RBUILD_FLAGS) -t install
