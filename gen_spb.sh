#!/bin/sh
SUFFIX=.sproto
filename=$1
cp $filename ../sproto_dump
basename=${filename%$SUFFIX}
newfilename=$basename.spb
cd ../sproto_dump
lua sprotodump.lua -spb $1 -o $newfilename 
cp $newfilename ../python-sproto/$newfilename
