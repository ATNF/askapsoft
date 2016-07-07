ftest: cpmanager.jar
	rbuild -ST -t functests

test: cpmanager.jar
	rbuild -ST -t test

all: cpmanager test ftest

clean:
	find . -type f -name '*junit-results.xml' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	find . -type f -name '*.pyc' -exec rm -f {} +
	find . -type d -name '*.ms' -exec rm -rf {} +
	rm -rf functests/data/
	rm -rf install/
	rm -f ./cpmanager*

cpmanager: cpmanager.jar

cpmanager.jar:
	rbuild -ST -t install
