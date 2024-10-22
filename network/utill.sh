#!/bin/bash
#cd network
#chmod u+x utill.sh
g++ -Iinclude include/Test/Test_utill.cc src/utill.cc -o Test_utill -lgtest -lglog
./Test_utill