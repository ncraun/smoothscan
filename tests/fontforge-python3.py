#!/usr/bin/python3

import imp
try:
    imp.find_module('fontforge')
    found = "yes"
except ImportError:
    found = "no"

print (found)
