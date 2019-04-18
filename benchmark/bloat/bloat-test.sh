#!/bin/bash

out=$1
stripped=${out}.stripped

ls -sh ${out}
cp ${out} ${stripped}
strip ${stripped}
ls -sh ${stripped}
