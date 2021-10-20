import ctypes
from time import sleep

lib = ctypes.CDLL("./volctrl.so")
lib.init.restype = ctypes.c_bool
lib.isInitialized.restype = ctypes.c_bool
lib.setVolume.restype = ctypes.c_bool
lib.getVolume.restype = ctypes.c_int
lib.setMute.restype = ctypes.c_bool
lib.mute.restype = ctypes.c_bool
lib.unmute.restype = ctypes.c_bool
lib.getMute.restype = ctypes.c_int

print(lib)

if not lib.init():
    print("Could not initialize library")
    exit(1)

vol = lib.getVolume()
print(f"Current volume: {vol}%")

print("Setting volume to 1/2 for 3 seconds...")
if not lib.setVolume(vol // 2):
    print("Could not set volume")
    exit(1)
sleep(3)
lib.setVolume(vol)

print("Test complete!")