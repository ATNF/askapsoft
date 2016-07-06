test:
	cd .. && rbuild -ST -t functests && cd -

clean:
	find . -type f -name '*.xml' -exec rm -f {} +
	find . -type f -name '*.log' -exec rm -f {} +
	find . -type f -name '*.pyc' -exec rm -f {} +
	rm -rf data/

rebuild:
	cd .. && rbuild -ST && cd -
