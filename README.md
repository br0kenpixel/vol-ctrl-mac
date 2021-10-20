# VolCtrlMac
Volume controller API for Python written in C++ for use on Macs. It uses Apple's CoreAudio API.

Allows getting/setting volume level (in %) as well as mute state on macOS. By using this library you have much better control over those things. And yes, you can control those things in "pure" Python, although it's usually done by emulating VOL+/-, (Un)Mute keypresses which obviously doesn't allow you to set the volume precisely. Furthermore, AFAIK there's no way to get the current volume level nor the mute state in "pure" Python.

Tested on macOS Catalina.

This library was designed to be used in Python 3. I designed it for a personal project which was supposed to allow controlling volume, mute state and playback on my Mac using the Novation Launchpad MIDI controller. You can use the ```ctypes``` module in Python to "import" this library. Just don't forget to set the function return types and preferably don't use Python's built-in types like `int`, `bool`, `float`, etc. Use `c_int`, `c_bool`, `c_float`, etc. from the `ctypes` module. Although using the build-in types should not cause any issues, but still.
