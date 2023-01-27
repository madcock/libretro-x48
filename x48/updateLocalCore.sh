#!/bin/bash

path="resources"
currentdir=$PWD

function convert {
	echo "convert $path$1"
	cd $path
	xxd -i $1 ../$1.c
	cd $currentdir
}


convert background.gif 

