#!/bin/sh

# extract version and release name and print to stdout
echo -n `egrep ' VERSION| RELEASENAME' src/include/main.h | \
	grep -v WELCOMEVER | awk '{print $3}'` | tr [\"\'] ' ' | \
	awk '{print $1 " " $2}'


