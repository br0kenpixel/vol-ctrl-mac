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
 * Copyright (c) 2022 Fábián Varga
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

extern "C" bool isInitialized(){
    return initialized;
}

std::vector<int> getValidChannels(AudioDeviceID *deviceID = NULL, int maxFailures = 3){
    std::vector<int> validChannels = {};
    if(deviceID == NULL) deviceID = &defaultOutputDeviceID;

    AudioObjectPropertyAddress propertyAddress = properties::volume;

    int channel = 0, errors = 0;
    while(errors < maxFailures){
        propertyAddress.mElement = channel;
        if(AudioObjectHasProperty(*deviceID, &propertyAddress)){
            validChannels.push_back(channel);
        } else errors++;
        channel++;
    }
    return validChannels;
}

extern "C" bool init(){
    UInt32 dataSize = sizeof(AudioDeviceID);
    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                 &properties::defaultOutputDevice,
                                                 0, NULL,
                                                 &dataSize, &defaultOutputDeviceID);
    if(result != kAudioHardwareNoError) return false;
    validChannelsForDefaultDevice = getValidChannels(&defaultOutputDeviceID);
    if(validChannelsForDefaultDevice.size() == 0) return false;
    initialized = true;
    return initialized;
}

extern "C" void deinit(){
    defaultOutputDeviceID = 0;
    validChannelsForDefaultDevice = {};
    initialized = false;
}

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

extern "C" bool setVolume(int volume_in_percent){
    Float32 volume = Float32(volume_in_percent) / 100;
    return setProperty(volume, properties::volume);
}

extern "C" int getVolume(){
    std::vector<Float32> volumes;
    bool error = !getProperty(volumes, properties::volume);
    if(error) return -1.0;

    Float32 volumeAvrg = std::accumulate(volumes.begin(), volumes.end(), 0.0f) / volumes.size();
    int x = int(roundf(volumeAvrg * 100.0f));
    return x;
}

extern "C" bool setMute(bool state){
    //Sometimes we have to use channel 0, idk why                                                              v
    return setProperty((UInt32)state, properties::mute) ? true : setProperty((UInt32)state, properties::mute, {0});
}

extern "C" bool mute(){
    return setMute(true);
}

extern "C" bool unmute(){
    return setMute(false);
}

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

//Demo
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