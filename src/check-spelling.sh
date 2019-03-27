#----------------------------------------------------------------------
# Description: check for spelling errors in main program before release.
# Author: Astor Newton Frain <astor@anewf.com>
# Created at: Wed Mar 27 17:07:26 -03 2019
# System: Linux 4.19.0-4-amd64 on x86_64
#
# Copyright (c) 2019 Astor Newton Frain
#
# This program is under GPL-2 and Apache-2.0 licenses.
#
#----------------------------------------------------------------------

#!/bin/bash

# Check for spell

TEST=$(spell -h)

[ "$TEST" ] || exit 1

# Check the dir

[ -e hello-destructor.c ] || exit 1

# Run spell

spell hello-destructor.c | sort -u
