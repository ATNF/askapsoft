clean:
	find . -type f -name '*.xml' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	find . -type f -name '*.pyc' -exec rm -f {} +
	rm -rf data/

test:
	cd .. && rbuild -ST -t functests && cd -
