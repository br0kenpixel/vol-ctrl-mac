#g++ -Wall -D__MACOSX_CORE__ -o test main.cpp \
#    -framework CoreMidi -framework CoreAudio -framework CoreFoundation -std=c++11

echo "WARN: If main() is undefined this command will fail"
g++ -D__MACOSX_CORE__ -framework CoreAudio -o test clib/volctrlmac.cpp -std=c++11
