#!/bin/bash

sudo rm /dev/hugepages/rtemap_*
./output/bin/standalone -t 3 
sudo rm /dev/hugepages/rtemap_*

exit
