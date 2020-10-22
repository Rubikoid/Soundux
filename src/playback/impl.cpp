#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>
#include "global.h"

namespace Soundux
{
    std::vector<ma_device_info> Playback::getPlaybackDevices()
    {
        ma_context context;

        if (ma_context_init(0, 0, 0, &context) != MA_SUCCESS)
        {
            std::cerr << "Failed to initialize context" << std::endl;
            return {};
        }

        ma_device_info *pPlayBackDeviceInfos;
        ma_uint32 deviceCount;

        ma_result result = ma_context_get_devices(&context, &pPlayBackDeviceInfos, &deviceCount, 0, 0);
        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to get playback devices!" << std::endl;
            return {};
        }

        std::vector<ma_device_info> playBackDevices;
        for (int i = 0; deviceCount > i; i++)
        {
            playBackDevices.push_back(pPlayBackDeviceInfos[i]);
        }

        ma_context_uninit(&context);

        return playBackDevices;
    }
    void Playback::setVolume(const ma_device_info &deviceInfo, float volume)
    {
        if (internal::usedDevices.find(deviceInfo.name) == internal::usedDevices.end())
        {
            std::cerr << "Device was not found" << std::endl;
            return;
        }
        internal::usedDevices[deviceInfo.name] = volume;
    }
    std::uint64_t Playback::playAudio(const std::string &file, const ma_device_info &deviceInfo)
    {
        static std::uint64_t counter = 0;

        if (internal::usedDevices.find(deviceInfo.name) == internal::usedDevices.end())
            internal::usedDevices.insert(std::make_pair(deviceInfo.name, 1.f));

        ma_decoder *decoder = new ma_decoder;
        ma_result result = ma_decoder_init_file(file.c_str(), 0, decoder);

        if (result != MA_SUCCESS)
        {
            std::cerr << "Failed to init decoder" << std::endl;
            return -1;
        }

        ma_device *device = new ma_device;
        ma_device_config config = ma_device_config_init(ma_device_type_playback);
        config.playback.format = decoder->outputFormat;
        config.playback.channels = decoder->outputChannels;
        config.sampleRate = decoder->outputSampleRate;
        config.dataCallback = internal::data_callback;
        config.playback.pDeviceID = &deviceInfo.id;
        config.pUserData = decoder;

        if (ma_device_init(0, &config, device) != MA_SUCCESS)
        {
            std::cerr << "Failed to open playback device" << std::endl;
            return -1;
        }
        if (ma_device_start(device) != MA_SUCCESS)
        {
            ma_device_uninit(device);
            ma_decoder_uninit(decoder);
            std::cerr << "Failed to start playback device" << std::endl;
            return -1;
        }

        internal::currentlyPlayingDevices->push_back({++counter, device, decoder});

        return counter;
    }
    void Playback::stop(const std::uint64_t &deviceId)
    {
        for (int i = 0; internal::currentlyPlayingDevices->size() > i; i++)
        {
            auto &device = internal::currentlyPlayingDevices->at(i);

            if (device.id == deviceId)
            {
                ma_device_uninit(device.device);
                ma_decoder_uninit(device.decoder);

                delete device.device;
                delete device.decoder;

                internal::currentlyPlayingDevices->erase(internal::currentlyPlayingDevices->begin() + i);

                break;
            }
        }
    }
    void Playback::stopAllAudio()
    {
        for (int i = 0; internal::currentlyPlayingDevices->size() > i; i++)
        {
            auto &player = internal::currentlyPlayingDevices->at(i);
            ma_device_uninit(player.device);
            ma_decoder_uninit(player.decoder);

            delete player.device;
            delete player.decoder;
        }
        internal::currentlyPlayingDevices->clear();
    }
    void Playback::internal::data_callback(ma_device *device, void *output, [[maybe_unused]] const void *input,
                                           std::uint32_t frameCount)
    {
        ma_decoder *decoder = reinterpret_cast<ma_decoder *>(device->pUserData);
        if (decoder == 0)
            return;

        if (usedDevices.find(device->playback.name) != usedDevices.end())
            device->masterVolumeFactor = usedDevices[device->playback.name];

        auto readFrames = ma_decoder_read_pcm_frames(decoder, output, frameCount);

        if (readFrames <= 0)
        {
            for (int i = 0; internal::currentlyPlayingDevices->size() > i; i++)
            {
                auto &dev = internal::currentlyPlayingDevices->at(i);
                if (dev.device == device)
                {
                    dev.finished = true;
                }
            }
        }
    }
} // namespace Soundux