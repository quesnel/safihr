#!/bin/bash


rm *svg *pdf

set -ex
for i in $(ls -1 *.gp); do /usr/local/bin/gnuplot "$i"; done
for i in $(ls -1 *.svg); do inkscape -z -T --export-pdf="$(basename $i .svg).pdf" "$i"; done
