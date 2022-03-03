import ctypes
from os import path

class VolCtrlMac:
    def __init__(self, clib_path=["./volctrl.so", "./../volctrl.so"]):
        if isinstance(clib_path, str):
            if not path.exists(clib_path):
                raise Exception("Could not find C library")
        elif isinstance(clib_path, (list, tuple)):
            for _path in clib_path:
                if path.exists(_path):
                    clib_path = _path
        else:
            raise TypeError("clib_path must be a string or a list of strings")
        if not isinstance(clib_path, str):
            raise Exception("Could not find C library")
        else:
            self.clib = ctypes.CDLL(clib_path)

        self.clib.init.restype = ctypes.c_bool
        self.clib.deinit.restype = None
        self.clib.isInitialized.restype = ctypes.c_bool
        self.clib.setVolume.restype = ctypes.c_bool
        self.clib.getVolume.restype = ctypes.c_int
        self.clib.setMute.restype = ctypes.c_bool
        self.clib.mute.restype = ctypes.c_bool
        self.clib.unmute.restype = ctypes.c_bool
        self.clib.getMute.restype = ctypes.c_int

        if self.clib.init() != True:
            raise Exception("Could not initialize C library")

        self.deinit = self.clib.deinit
        self.isInitialized = self.clib.isInitialized
        self.getVolume = self.clib.getVolume
        self.getMute = self.clib.getMute

    def reinit(self):
        try:
            self.__ensure_initialized()
        except:
            if self.clib.init() != True:
                raise Exception("Could not initialize C library")
            return
        raise Exception("C library is already initialized")

    def __ensure_initialized(self):
        if not self.isInitialized():
            raise Exception("C library is not initialized")

    def setVolume(self, vol):
        self.__ensure_initialized()
        if vol not in range(0, 101):
            raise ValueError(r"Volume must be between 0 and 100 (%)")
        else:
            if not self.clib.setVolume(vol):
                raise Exception("Could not set volume")

    def setMute(self, mute):
        self.__ensure_initialized()
        if mute not in (True, False, 1, 0):
            raise ValueError("Mute must be True or False")
        else:
            if not self.clib.setMute(mute):
                raise Exception("Could not set mute state")

    def mute(self):
        self.setMute(True)

    def unmute(self):
        self.setMute(False)


if __name__ == "__main__":
    vcm = VolCtrlMac()
    print("Ready.")