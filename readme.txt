**************
Dependencies
**************

zeromq
--------------
sudo apt-get install libzmq3-dev


C++ boost libraries
--------------------
sudo apt-get install libboost-all-dev


CMAKE
--------------------
sudo apt-get install cmake


External libraries included in source tree
------------------------------------------
g3log
cppzmq (header only library)
cmdline (header only library)
disruptor


External libraries not included in source tree
------------------------------------------
USB-4750 DIO card drivers (see dedicated doc for more information)


*********************
Build instructions
*********************
Compiling falcon has only been tested with GNU g++ compiler.
Version 5 is required, as it contains all C++11 libraries (see doc/g++5_how_to_install.txt for info about how to install g++ v5).
So, to compile issue the following commands while in the falcon root directory: 

mkdir build
cd build
cmake -DCMAKE_CXX_COMPILER=`which g++-5` ..
make


***************************
Installation instructions
***************************

cd build/src
cp falcon ~/bin
sudo setcap 'cap_sys_nice=pe' ~/bin/falcon
