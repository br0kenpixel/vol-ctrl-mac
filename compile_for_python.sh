g++ -c -c -fPIC -D__MACOSX_CORE__ main.cpp -o volctrl.o -std=c++11
g++ -shared -Wl -framework CoreAudio -o volctrl.so volctrl.o