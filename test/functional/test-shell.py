#!/usr/bin/env python3

#
# Launch this with command from test/functional directory
# > python3 -i test-shell.py
#

from test_framework.test_shell import TestShell

test = TestShell().setup()
node = test.nodes[0]
