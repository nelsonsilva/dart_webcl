#!/bin/sh
svn co https://projects.developer.nokia.com/svn/webcl/trunk/wrapper@56 lib/wrapper
CXXFLAGS=-fPIC INCLUDES=-I/opt/cuda/include/ make -C lib/wrapper
