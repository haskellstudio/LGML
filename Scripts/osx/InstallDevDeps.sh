#!/usr/bin/bash
pwd;
# this script install :
#       JUCE in sibling directory of LGML
#       compile rubberband locally (third_party/rubberband)

BASEDIR=$(dirname "$0") 
cd $BASEDIR
cd ../../..

# install JUCE if needed
if [ ! -d "JUCE" ]; then
	curl -L https://github.com/julianstorer/JUCE/archive/master.zip > JUCE.zip;
	tar -xzf JUCE.zip;
	mv JUCE-master JUCE;
fi



cd $BASEDIR
cd ../../third_party/rubberband
# ./configure
mkdir -p lib
make -f Makefile.osx CPPFLAGS=-DNO_THREADING static