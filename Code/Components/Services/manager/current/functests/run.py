#!/usr/bin/env python
""" Simpler Python script for running multiple functional tests. The primary
benefit over the run.sh script is that adding new tests requires just a single
new line rather than a whole bunch of duplicated Bash code.
"""

import subprocess


if __name__ == "__main__":

    # Add tests here, using "name":"path"
    tests = {
        "Mock Ingest": "./dummyingest",
        "Real Ingest": "./processingest",
    }

    results = {}
    failure_count = 0

    # execute all tests
    print("Running {0} tests ...\n".format(len(tests)))
    for key, value in tests.iteritems():
        print("{0}\n----------------------------------------------".format(key))
        command = value + "/run.sh"
        status = subprocess.call(command)
        results[key] = "{0} ({1})".format(
            "PASS" if status == 0 else "FAIL",
            status)
        if status != 0:
            failure_count+=1
        print("\n")

    # display results
    print("Result Summary:")
    print("======================================")
    for key, value in results.iteritems():
        print("{0}\t\t{1}".format(key, value))

    # return the failure count
    exit(failure_count)
