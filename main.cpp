#include <CoreAudio/CoreAudio.h>
#include <math.h>
#include <vector>
#include <numeric>

#pragma GCC diagnostic ignored "-Wsizeof-pointer-div"
#define COMPILE_FOR_PYTHON

#ifndef COMPILE_FOR_PYTHON
    #include <iostream>
    #include <unistd.h>
    #include <cstdlib>
    using namespace std;
#endif

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
};

AudioDeviceID defaultOutputDeviceID;

extern "C" bool isInitialized(){
    AudioObjectPropertyAddress propertyAddress = { 
        kAudioDevicePropertyVolumeScalar, //mSelector
        kAudioDevicePropertyScopeOutput, //mScope
        0 //mElement
    };

    bool master_available, first_channel_available;
    master_available = AudioObjectHasProperty(defaultOutputDeviceID, &propertyAddress);
    propertyAddress.mElement = 1;
    first_channel_available = AudioObjectHasProperty(defaultOutputDeviceID, &propertyAddress);

    return master_available || first_channel_available;
}

extern "C" bool init(){
    AudioObjectPropertyAddress getDefaultOutputDevicePropertyAddress = {
        kAudioHardwarePropertyDefaultOutputDevice,
        kAudioObjectPropertyScopeGlobal,
        kAudioObjectPropertyElementMaster
    };

    UInt32 volumedataSize = sizeof(defaultOutputDeviceID);
    OSStatus result = AudioObjectGetPropertyData(kAudioObjectSystemObject,
                                                &getDefaultOutputDevicePropertyAddress,
                                                0, NULL,
                                                &volumedataSize, &defaultOutputDeviceID);

    return result == kAudioHardwareNoError;
}

std::vector<int> getValidChannels(int maxChannels = 32, int maxFailures = 3){
    std::vector<int> validChannels = {};
    validChannels.reserve(maxChannels);
    if(!isInitialized()) return validChannels;

    AudioObjectPropertyAddress propertyAddress = {
        kAudioDevicePropertyVolumeScalar, 
        kAudioDevicePropertyScopeOutput,
        0
    };

    int channel = 0;
    int failures = 0;
    int index = 0;
    for(channel = 0; channel <= maxChannels; channel++){
        if(failures > maxFailures){
            break;
        }
        propertyAddress.mElement = channel;
        if(!AudioObjectHasProperty(defaultOutputDeviceID, &propertyAddress)){
            //cout << "Channel " << channel << " is not valid" << endl;
            if(channel != 0) failures++;
        } else {
            validChannels.push_back(channel);
            //cout << "Channel " << channel << " is valid" << endl;
        }
    }

    //cout << "Looks like there are/is " << index << " valid channel(s)" << endl;

    return validChannels;
}

template <typename UniversalDataType>
bool setProperty(UniversalDataType data, AudioObjectPropertyAddress propertyAddr, std::vector<int> channels = {}){
    UInt32 dataSize = sizeof(data);
    if(channels.empty()) channels = getValidChannels();
    int validChannelCount = channels.size();

    std::vector<bool> statuses;
    OSStatus result;
    for(int i = 0; i < validChannelCount; i++) {
        propertyAddr.mElement = channels[i];
        result = AudioObjectSetPropertyData(defaultOutputDeviceID,
                                            &propertyAddr,
                                            0, NULL, dataSize, &data
                                           );
        statuses.push_back(result == kAudioHardwareNoError);
    }
    bool error = !std::all_of(statuses.begin(), statuses.end(), [](bool v) { return v; });

    return !error;
}

template <typename UniversalDataType>
bool getProperty(std::vector<UniversalDataType> &buffer, AudioObjectPropertyAddress propertyAddr, std::vector<int> channels = {}){
    UniversalDataType data;
    UInt32 dataSize = sizeof(data);
    if(channels.empty()) channels = getValidChannels();
    int validChannelCount = channels.size();

    std::vector<bool> statuses;
    std::vector<UniversalDataType> dataCollection;
    OSStatus result;
    for(int i = 0; i < validChannelCount; i++) {
        propertyAddr.mElement = channels[i];
        result = AudioObjectGetPropertyData(defaultOutputDeviceID,
                                            &propertyAddr,
                                            0, NULL, &dataSize, &data
                                           );
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