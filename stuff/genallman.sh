#!/bin/bash

# Generate several manpages at the same time.
# C 2014 Joao Eriberto Mota Filho <eriberto@debian.org>
#
# You can use this code in the same terms of the BSD-3-clause license or,
# optionally, in the same terms of the license used in debian/ directory
# in this Debian package. Please, to the last option, refer the package
# name when using.
#
# This script uses txt2man. You need 2 files: program_name.txt and
# program_name.header.
#
# The program_name.header must be use this structure:
#
# .TH <program_name> "<manpage_level>"  "<date>" "<program_name_upper_case> <program_version>" "<program_description>"
#
# Example:
#
# .TH mac-robber "1"  "May 2013" "MAC-ROBBER 1.02" "collects data about allocated files in mounted filesystems"

for NAME in $(ls | grep header | cut -d'.' -f1)
do
    LEVEL=$(cat $NAME.header | cut -d" " -f3 | tr -d '"')
    cat $NAME.header > $NAME.$LEVEL
    txt2man $NAME.txt | grep -v '^.TH ' >> $NAME.$LEVEL
    cp -f $NAME.$LEVEL ../doc
done
