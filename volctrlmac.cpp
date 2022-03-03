/*
 *
 * Launchpad VolControlMac v2.
 *
 *
 * This is the part responsible for providing low-level
 * access to audio devices on a Mac.
 * This is not possible directly in Python, so this file
 * is used with Python's ctypes module.
 *
 * Some things, like detecting channel count are
 * probably not done the way they should be, but there's
 * very little to no information on how to use CoreAudio.
 *
 *
 * Copyright (c) 2022 Fábián Varga (br0kenpixel)
*/

#include <CoreAudio/CoreAudio.h>
#include <math.h>
#include <vector>
#include <numeric>

//Define to make this file work when compiling to a shared library
//If not defined, you can run this file to test the features.
#define COMPILE_FOR_PYTHON

#ifndef COMPILE_FOR_PYTHON
    //Wel'll include some extra headers for debugging
    #include <iostream>
    #include <unistd.h>
    #include <cstdlib>
    using namespace std;
#endif

//Here, we'll store some audio device property addresses
namespace properties {
    //Volume control
    const AudioObjectPropertyAddress volume = {
        kAudioDevicePropertyVolumeScalar, //mSelector
        kAudioDevicePropertyScopeOutput, //mScope
        0 //mElement
    };
    //Mute control
    const AudioObjectPropertyAddress mute = { 
        kAudioDevicePropertyMute,
        kAudioDevicePropertyScopeOutput,
        0
    };
    //Default output device
    const AudioObjectPropertyAddress defaultOutputDevice = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };
};

AudioDeviceID defaultOutputDeviceID = 0;                //ID of the default output device
std::vector<int> validChannelsForDefaultDevice = {};    //List of valid channels for the default output device
bool initialized = false;                               //Just to know wheather we got the default device ID

/**
 * Check if the library is initialized.
 *
 * @result - bool wheather the library is initialized or not
 */
extern "C" bool isInitialized(){
    return initialized;
}

/**
 * Get a list of valid channels for the default output device.
 * If deviceID is NULL, it will be set to the default output device.
 *
 * @param deviceID - output device
 * @param maxFailures - number of errors after which the scan is stopped
 * @result - list (vector) of valid channels
 */
std::vector<int> getValidChannels(AudioDeviceID *deviceID = NULL, int maxFailures = 3){
    std::vector<int> validChannels = {};
    if(deviceID == NULL) deviceID = &defaultOutputDeviceID;

    //During the check we'll be trying to see if the channel has a
    //volume level property
    AudioObjectPropertyAddress propertyAddress = properties::volume;

    int channel = 0, errors = 0;
    while(errors < maxFailures){
        //Cycle trough channels until the last [maxFailures] channels
        //are invalid
        propertyAddress.mElement = channel;
        if(AudioObjectHasProperty(*deviceID, &propertyAddress)){
            //Channel is valid, add it to the list
            validChannels.push_back(channel);
        } else errors++;
        channel++;
    }
    return validChannels;
}

/**
 * Initialize the library.
 *
 * @result - bool wheather the library was initialized successfully
 */
extern "C" bool init(){
    UInt32 dataSize = sizeof(AudioDeviceID);
    //Find the default output device
    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                 &properties::defaultOutputDevice,
                                                 0, NULL,
                                                 &dataSize, &defaultOutputDeviceID);
    if(result != kAudioHardwareNoError) return false;

    //Get a list of valid channels
    validChannelsForDefaultDevice = getValidChannels(&defaultOutputDeviceID);
    if(validChannelsForDefaultDevice.size() == 0) return false;
    initialized = true;
    return initialized;
}

/**
 * Deinitialize the library.
 */
extern "C" void deinit(){
    defaultOutputDeviceID = 0;
    validChannelsForDefaultDevice = {};
    initialized = false;
}

/**
 * Set a property of the default output device.
 * This function is universal, thus it can be used to
 * set volume level or set the mute state.
 * 
 * This is an internal function, which will not be
 * accessible from Python.
 *
 * @param data - buffer to read from (value to set)
 * @param propertyAddr - address of the property
 * @param channels - list of valid channels
 * @result - wheather the set failed or succeeded
 */
template <typename UniversalDataType>
bool setProperty(UniversalDataType data, AudioObjectPropertyAddress propertyAddr, std::vector<int> channels = validChannelsForDefaultDevice){
    UInt32 dataSize = sizeof(data);

    std::vector<bool> statuses;
    OSStatus result;
    for(int i = 0; i < channels.size(); i++) {
        propertyAddr.mElement = channels[i];
        result = AudioObjectSetPropertyData(defaultOutputDeviceID,
                                            &propertyAddr,
                                            0, NULL, dataSize, &data);
        statuses.push_back(result == kAudioHardwareNoError);
    }
    bool error = !std::all_of(statuses.begin(), statuses.end(), [](bool v) { return v; });

    return !error;
}

/**
 * Get the value of a property of the default output device.
 * This function is universal, thus it can be used to
 * get the volume level or get the mute state.
 * 
 * This is an internal function, which will not be
 * accessible from Python.
 *
 * @param buffer - buffer to write to (should be a vector)
 * @param propertyAddr - address of the property
 * @param channels - list of valid channels
 * @result - wheather the set failed or succeeded
 */
template <typename UniversalDataType>
bool getProperty(std::vector<UniversalDataType> &buffer, AudioObjectPropertyAddress propertyAddr, std::vector<int> channels = validChannelsForDefaultDevice){
    UniversalDataType data;
    UInt32 dataSize = sizeof(data);

    std::vector<bool> statuses;
    std::vector<UniversalDataType> dataCollection;
    OSStatus result;
    for(int i = 0; i < channels.size(); i++) {
        propertyAddr.mElement = channels[i];
        result = AudioObjectGetPropertyData(defaultOutputDeviceID,
                                            &propertyAddr,
                                            0, NULL, &dataSize, &data);
        statuses.push_back(result == kAudioHardwareNoError);
        if(result == kAudioHardwareNoError) {
            dataCollection.push_back(data);
        }
    }
    bool error = !std::all_of(statuses.begin(), statuses.end(), [](bool v) { return v; });
    if(error) return false;

    for(UniversalDataType d_t : dataCollection){
        buffer.push_back(d_t);
    }

    return true;
}

/**
 * Set the volume level of the default output device.
 *
 * @param volume_in_percent - volume level (0-100)
 * @result - wheather the set failed or succeeded
 */
extern "C" bool setVolume(int volume_in_percent){
    Float32 volume = Float32(volume_in_percent) / 100;
    return setProperty(volume, properties::volume);
}

/**
 * Get the volume level of the default output device.
 * If the output device has multiple channels and they
 * are set to a different volume level then the
 * averrage is returned.
 *
 * @result - volume level (0-100%)
 */
extern "C" int getVolume(){
    std::vector<Float32> volumes;
    bool error = !getProperty(volumes, properties::volume);
    if(error) return -1.0;

    Float32 volumeAvrg = std::accumulate(volumes.begin(), volumes.end(), 0.0f) / volumes.size();
    int x = int(roundf(volumeAvrg * 100.0f));
    return x;
}

/**
 * Set the mute state of the default output device.
 * 
 * @param state - muted/unmuted (1/0)
 * @result - wheather the set failed or succeeded
 */
extern "C" bool setMute(bool state){
    //Sometimes we have to use channel 0, idk why                                                              v
    return setProperty((UInt32)state, properties::mute) ? true : setProperty((UInt32)state, properties::mute, {0});
}

/**
 * Mute the audio output of the default device.
 * This is an alias to setMute(true).
 * 
 * @result - wheather the set failed or succeeded
 */
extern "C" bool mute(){
    return setMute(true);
}

/**
 * Unmute the audio output of the default device.
 * This is an alias to setMute(false).
 * 
 * @result - wheather the set failed or succeeded
 */
extern "C" bool unmute(){
    return setMute(false);
}

/**
 * Get the mute state of the default output device.
 * If the output device has multiple channels and they
 * are not all muted/unmuted then the mute state of the
 * last channel is returned.
 *
 * @result - mute state (0/1)
 */
extern "C" int getMute(){
    //Warning: Do NOT use bool vector, must be int!
    std::vector<int> muteStates;
    bool error = getProperty(muteStates, properties::mute) ? false : !getProperty(muteStates, properties::mute, {0});
    if(error) return -1;

    bool finalState = true;
    for(int muteState_as_int : muteStates){
        bool muteState = (bool)muteState_as_int;
        finalState = finalState && muteState;
    }

    return (int)finalState;
}

#ifndef COMPILE_FOR_PYTHON

/* DEMO CODE  */

int main(){
    cout << "Welcome" << endl;
    cout << "Initializing Apple CoreAudio..." << endl;
    bool init_error = init();

    if (!init_error && isInitialized()) {
        cout << "Initialization failed" << endl;
        return 1;
    } else {
        cout << "Initialization OK" << endl;
    }

    std::vector<int> channels = getValidChannels();
    int channelCount = channels.size();
    cout << "Available channels: " << channelCount << " -> ";
    for(int i = 0; i < channelCount; i++) {
        cout << channels[i];
        if(i != channelCount - 1) cout << ", ";
    }
    cout << endl;

    int volume = getVolume();
    if(volume == -1){
        cout << "Error getting volume" << endl;
        return 1;
    } else {
        cout << "Current volume level: " << volume << "%" << endl;
    }
    
    int muted = getMute();
    if(muted == -1){
        cout << "Error getting mute state" << endl;
        return -1;
    } else {
        cout << "Muted state: " << muted << endl;
    }
    cout << defaultOutputDeviceID << endl;
    deinit();

    /*
    cout << "Setting volume to" << volume / 2 << "percent for 3 seconds" << endl;
    bool error = !setVolume(volume / 2);
    if(error){
        cout << "Error setting volume" << endl;
        return -1;
    } else {
        cout << "Returning volume level in 3 seconds..." << endl;
        sleep(3);
        setVolume(volume);
        cout << "Volume reset" << endl;
    }
    */

    return 0;
}
#endif