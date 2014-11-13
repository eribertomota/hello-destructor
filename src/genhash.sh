#!/bin/bash

# This script calculates the hashes
# to ensure the integrity.
#
# Copyright 2013 John Fritz Hall
#
# You can use this script under the
# MIT license.

echo "Calculating hashes..."

md5sum he* ra* > hashes

echo "Ok, done."
