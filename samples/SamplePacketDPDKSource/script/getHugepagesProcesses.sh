#!/bin/bash

#!/bin/bash

## Copyright (C) 2011, 2017  International Business Machines Corporation
## All Rights Reserved

################### functions used in this script #############################

die() { echo ; echo -e "\e[1;31m$*\e[0m" >&2 ; exit 1 ; }
step() { echo ; echo -e "\e[1;34m$*\e[0m" ; }

################################################################################

step "checking 'hugepages' usage ..."
sudo grep huge /proc/*/numa_maps

pids=$( sudo grep huge /proc/*/numa_maps | cut -d '/' -f 3 | sort -n | uniq )
[[ -z $pids ]] && exit 0

step "checking 'hugepages' users ..."
ps -o user,pid,ppid,cmd ${pids[@]}

exit 0
