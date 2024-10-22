#!/bin/bash
#cd network
#chmod u+x Inet_address.sh
g++ -Iinclude include/Test/Test_Inet_address.cc src/Inet_address.cc src/SocketOps.cc -o Test_Inet_address -lgtest -lglog
./Test_Inet_address