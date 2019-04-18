#!/bin/bash

f=$1
i=$2
out=$3

sed $f -e "s/do_scan/do_scan$i/" -e "s/42/$i/" > $out
