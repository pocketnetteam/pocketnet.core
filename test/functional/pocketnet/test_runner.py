#!/usr/bin/env python3
# Copyright (c) 2023 The Pocketnet developers
# Distributed under the Apache 2.0 software license, see the accompanying
# https://www.apache.org/licenses/LICENSE-2.0
"""
Test Runner to execute the entire Pocketnet test suite
Launch this from 'test/functional/pocketnet' directory
"""

import configparser
import logging
import tempfile
import time
import unittest

from datetime import datetime

import os
import sys
import pathlib

sys.path.insert(0, str(pathlib.Path(__file__).parent.parent))

from test_runner import TestHandler, TestResult, print_results


BASE_SCRIPTS = [
    "account_delete.py",
    "badges.py",
    # 'content_collections.py',
    "moderation_jury_no_ban.py",
    "moderation_jury.py",
    "chainbuilder_full_activity.py",
]


NON_SCRIPTS = [
    # These are python files that live in the functional tests
    # directory, but are not test scripts.
    "test_runner.py",
]

BOLD = ('\033[0m', '\033[1m')


def discover():
    return BASE_SCRIPTS

def tempdir():
    now = datetime.now().strftime("%Y%m%d_%H%M%S")
    tmpdir = os.path.join(tempfile.gettempdir(), f"test_runner_pocketnet_{now}")
    os.makedirs(tmpdir)
    return tmpdir

def get_config():
    config = configparser.ConfigParser()
    configfile = os.path.abspath(os.path.dirname(__file__)) + "/../../config.ini"
    config.read_file(open(configfile, encoding="utf8"))
    return config

def execute(job_queue, test_num, test_count):
    test_result, testdir, stdout, stderr = job_queue.get_next()
    done_str = "{}/{} - {}{}{}".format(test_num + 1, test_count, BOLD[1], test_result.name, BOLD[0])
    if test_result.status == "Passed":
        logging.debug("%s passed, Duration: %s s" % (done_str, test_result.time))
    elif test_result.status == "Skipped":
        logging.debug("%s skipped" % (done_str))
    else:
        print("%s failed, Duration: %s s\n" % (done_str, test_result.time))
        print(BOLD[1] + 'stdout:\n' + BOLD[0] + stdout + '\n')
        print(BOLD[1] + 'stderr:\n' + BOLD[0] + stderr + '\n')
    return test_result

def setup_logging():
    logging_level = logging.DEBUG
    logging.basicConfig(format='%(message)s', level=logging_level)

def run_tests():
    config = get_config()
    src_dir = config["environment"]["SRCDIR"]
    build_dir = config["environment"]["BUILDDIR"]

    job_queue = TestHandler(
        num_tests_parallel=3,
        tests_dir=os.path.join(src_dir, "test/functional/pocketnet/"),
        tmpdir=tempdir(),
        test_list=discover(),
        flags=[],
        use_term_control=False,
    )

    start_time = time.time()
    test_count = len(job_queue.test_list)

    print("Running Pocketnet tests configured in BASE_SCRIPTS...")

    for test_num in range(test_count):
        result = execute(job_queue, test_num, test_count)
        print(result, f"Runtime: {time.time() - start_time}")

def main():
    setup_logging()
    run_tests()


if __name__ == "__main__":
    main()
