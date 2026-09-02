#include "Audio.hpp"
#include <memory>

#define MINIAUDIO_IMPLEMENTATION
#include <miniaudio.h>

#define TSF_IMPLEMENTATION
#include "tsf.h"

#define TML_IMPLEMENTATION
#include "tml.h"

#include <cstring>
#include <iostream>
#include <unordered_map>
#include <vector>

namespace tempest {

struct MidiSource {
    ma_data_source_base base;
    struct AudioImpl* impl;
};

struct AudioImpl {
    ma_engine engine{};
    bool engineOk = false;
    tsf* font = nullptr;
    ma_mutex mutex{};
    bool mutexOk = false;
    MidiSource source{};
    bool sourceOk = false;
    ma_sound musicSound{};
    bool musicSoundOk = false;
    int sampleRate = 44100;

    std::unordered_map<std::string, std::string> sfxPath;
    std::unordered_map<std::string, std::string> musicPath;
    std::unordered_map<std::string, tml_message*> midiCache;

    tml_message* midiHead = nullptr;
    tml_message* midiCur = nullptr;
    double msec = 0.0;
    unsigned int midiLengthMs = 0;
    bool playing = false;
    std::string musicName;
    float musicVolume = 0.45f;

    void applyMidi(const tml_message* msg) {
        if (!font || !msg) return;
        switch (msg->type) {
        case TML_PROGRAM_CHANGE:
            tsf_channel_set_presetnumber(font, msg->channel, msg->program, (msg->channel == 9));
            break;
        case TML_NOTE_ON:
            tsf_channel_note_on(font, msg->channel, msg->key, msg->velocity / 127.0f);
            break;
        case TML_NOTE_OFF:
            tsf_channel_note_off(font, msg->channel, msg->key);
            break;
        case TML_PITCH_BEND:
            tsf_channel_set_pitchwheel(font, msg->channel, msg->pitch_bend);
            break;
        case TML_CONTROL_CHANGE:
            tsf_channel_midi_control(font, msg->channel, msg->control, msg->control_value);
            break;
        default:
            break;
        }
    }

    void loopSong() {
        if (!font) return;
        tsf_note_off_all(font);
        midiCur = midiHead;
        msec = 0.0;
    }

    void render(float* out, ma_uint32 frameCount) {
        if (!out) {
            std::vector<float> tmp(static_cast<size_t>(frameCount) * 2u);
            render(tmp.data(), frameCount);
            return;
        }
        if (!font || !playing) {
            std::memset(out, 0, sizeof(float) * frameCount * 2u);
            return;
        }
        float* stream = out;
        ma_uint32 left = frameCount;
        const double msPerSample = 1000.0 / static_cast<double>(sampleRate);
        while (left) {
            ma_uint32 block = TSF_RENDER_EFFECTSAMPLEBLOCK;
            if (block > left) block = left;
            msec += static_cast<double>(block) * msPerSample;
            if (midiHead && midiLengthMs > 0 && msec >= static_cast<double>(midiLengthMs)) {
                loopSong();
            }
            while (midiCur && msec >= midiCur->time) {
                applyMidi(midiCur);
                midiCur = midiCur->next;
            }
            if (!midiCur && midiHead) {
                loopSong();
            }
            tsf_render_float(font, stream, static_cast<int>(block), 0);
            stream += block * 2u;
            left -= block;
        }
    }
};

static ma_result midi_read(ma_data_source* pDataSource, void* pFramesOut, ma_uint64 frameCount, ma_uint64* pFramesRead) {
    auto* src = reinterpret_cast<MidiSource*>(pDataSource);
    AudioImpl* impl = src->impl;
    if (pFramesRead) *pFramesRead = 0;
    if (!impl || frameCount == 0) return MA_INVALID_ARGS;
    ma_mutex_lock(&impl->mutex);
    impl->render(static_cast<float*>(pFramesOut), static_cast<ma_uint32>(frameCount));
    ma_mutex_unlock(&impl->mutex);
    if (pFramesRead) *pFramesRead = frameCount;
    return MA_SUCCESS;
}

static ma_result midi_seek(ma_data_source*, ma_uint64) { return MA_NOT_IMPLEMENTED; }

static ma_result midi_format(ma_data_source* pDataSource, ma_format* pFormat, ma_uint32* pChannels, ma_uint32* pSampleRate,
                             ma_channel* pChannelMap, size_t channelMapCap) {
    auto* src = reinterpret_cast<MidiSource*>(pDataSource);
    if (pFormat) *pFormat = ma_format_f32;
    if (pChannels) *pChannels = 2;
    if (pSampleRate) *pSampleRate = static_cast<ma_uint32>(src->impl ? src->impl->sampleRate : 44100);
    if (pChannelMap && channelMapCap >= 2) {
        pChannelMap[0] = MA_CHANNEL_FRONT_LEFT;
        pChannelMap[1] = MA_CHANNEL_FRONT_RIGHT;
    }
    return MA_SUCCESS;
}

static ma_result midi_cursor(ma_data_source*, ma_uint64* pCursor) {
    if (pCursor) *pCursor = 0;
    return MA_NOT_IMPLEMENTED;
}

static ma_result midi_length(ma_data_source*, ma_uint64* pLength) {
    if (pLength) *pLength = 0;
    return MA_NOT_IMPLEMENTED;
}

static ma_result midi_set_looping(ma_data_source*, ma_bool32) { return MA_SUCCESS; }

static ma_data_source_vtable g_midi_vtable = {
    midi_read,
    midi_seek,
    midi_format,
    midi_cursor,
    midi_length,
    midi_set_looping,
    MA_DATA_SOURCE_SELF_MANAGED_RANGE_AND_LOOP_POINT};

Audio::Audio() : impl_(std::make_unique<AudioImpl>()) {}
Audio::~Audio() { destroy(); }

bool Audio::init(const std::string& root) {
    AudioImpl& i = *impl_;
    ma_engine_config cfg = ma_engine_config_init();
    if (ma_engine_init(&cfg, &i.engine) != MA_SUCCESS) {
        std::cerr << "miniaudio engine init failed (continuing silent)\n";
        return true;
    }
    i.engineOk = true;
    i.sampleRate = static_cast<int>(ma_engine_get_sample_rate(&i.engine));
    if (i.sampleRate < 1) i.sampleRate = 44100;

    if (ma_mutex_init(&i.mutex) != MA_SUCCESS) {
        std::cerr << "audio mutex init failed (continuing silent music)\n";
        return true;
    }
    i.mutexOk = true;

    const std::string sf2 = root + "/assets/music/gm.sf2";
    i.font = tsf_load_filename(sf2.c_str());
    if (!i.font) {
        std::cerr << "SoundFont missing or unreadable at " << sf2 << " (music silent; SFX still work)\n";
    } else {
        tsf_set_output(i.font, TSF_STEREO_INTERLEAVED, i.sampleRate, -10.0f);
        tsf_set_max_voices(i.font, 96);
        for (int ch = 0; ch < 16; ++ch) {
            tsf_channel_set_presetnumber(i.font, ch, 0, ch == 9);
        }
        tsf_channel_set_bank_preset(i.font, 9, 128, 0);
    }

    i.source.impl = &i;
    ma_data_source_config dsc = ma_data_source_config_init();
    dsc.vtable = &g_midi_vtable;
    if (ma_data_source_init(&dsc, &i.source.base) != MA_SUCCESS) {
        std::cerr << "MIDI data source init failed (music silent)\n";
        return true;
    }
    i.sourceOk = true;

    const ma_uint32 flags = MA_SOUND_FLAG_NO_PITCH | MA_SOUND_FLAG_NO_SPATIALIZATION;
    if (ma_sound_init_from_data_source(&i.engine, &i.source.base, flags, nullptr, &i.musicSound) != MA_SUCCESS) {
        std::cerr << "MIDI sound node init failed (music silent)\n";
        return true;
    }
    i.musicSoundOk = true;
    ma_sound_set_looping(&i.musicSound, MA_TRUE);
    return true;
}

void Audio::destroy() {
    if (!impl_) return;
    AudioImpl& i = *impl_;
    if (i.mutexOk) ma_mutex_lock(&i.mutex);
    i.playing = false;
    i.midiCur = nullptr;
    i.midiHead = nullptr;
    if (i.font) tsf_note_off_all(i.font);
    if (i.mutexOk) ma_mutex_unlock(&i.mutex);

    if (i.musicSoundOk) {
        ma_sound_uninit(&i.musicSound);
        i.musicSoundOk = false;
    }
    if (i.sourceOk) {
        ma_data_source_uninit(&i.source.base);
        i.sourceOk = false;
    }
    for (auto& kv : i.midiCache) {
        if (kv.second) tml_free(kv.second);
    }
    i.midiCache.clear();
    if (i.font) {
        tsf_close(i.font);
        i.font = nullptr;
    }
    if (i.mutexOk) {
        ma_mutex_uninit(&i.mutex);
        i.mutexOk = false;
    }
    if (i.engineOk) {
        ma_engine_uninit(&i.engine);
        i.engineOk = false;
    }
}

void Audio::loadSfx(const std::string& name, const std::string& path) { impl_->sfxPath[name] = path; }

void Audio::loadMusic(const std::string& name, const std::string& path) {
    AudioImpl& i = *impl_;
    i.musicPath[name] = path;
    if (i.midiCache.count(name)) return;
    tml_message* midi = tml_load_filename(path.c_str());
    if (!midi) {
        std::cerr << "MIDI missing or unreadable at " << path << " (that cue will be silent)\n";
        return;
    }
    i.midiCache[name] = midi;
}

void Audio::play(const std::string& name, float volume) {
    AudioImpl& i = *impl_;
    if (!i.engineOk) return;
    auto it = i.sfxPath.find(name);
    if (it == i.sfxPath.end()) return;
    (void)volume;
    ma_engine_play_sound(&i.engine, it->second.c_str(), nullptr);
}

void Audio::playMusic(const std::string& name, float volume) {
    AudioImpl& i = *impl_;
    if (!i.engineOk) return;
    auto pit = i.musicPath.find(name);
    if (pit == i.musicPath.end()) return;

    if (!i.midiCache.count(name) || !i.midiCache[name]) {
        tml_message* midi = tml_load_filename(pit->second.c_str());
        if (!midi) {
            std::cerr << "MIDI missing or unreadable at " << pit->second << " (music silent)\n";
            return;
        }
        i.midiCache[name] = midi;
    }

    if (!i.font) {
        std::cerr << "SoundFont not loaded; music silent\n";
        return;
    }

    tml_message* head = i.midiCache[name];
    unsigned int lengthMs = 0;
    tml_get_info(head, nullptr, nullptr, nullptr, nullptr, &lengthMs);

    if (i.mutexOk) ma_mutex_lock(&i.mutex);
    if (i.musicName == name && i.playing) {
        i.musicVolume = volume;
        tsf_set_volume(i.font, volume);
        if (i.mutexOk) ma_mutex_unlock(&i.mutex);
        return;
    }
    tsf_note_off_all(i.font);
    i.midiHead = head;
    i.midiCur = head;
    i.msec = 0.0;
    i.midiLengthMs = lengthMs;
    i.playing = true;
    i.musicName = name;
    i.musicVolume = volume;
    tsf_set_volume(i.font, volume);
    if (i.mutexOk) ma_mutex_unlock(&i.mutex);

    if (i.musicSoundOk) {
        ma_sound_set_volume(&i.musicSound, 1.0f);
        ma_sound_start(&i.musicSound);
    }
}

void Audio::stopMusic() {
    AudioImpl& i = *impl_;
    if (i.mutexOk) ma_mutex_lock(&i.mutex);
    i.playing = false;
    i.musicName.clear();
    i.midiCur = nullptr;
    i.midiHead = nullptr;
    if (i.font) tsf_note_off_all(i.font);
    if (i.mutexOk) ma_mutex_unlock(&i.mutex);
    if (i.musicSoundOk) ma_sound_stop(&i.musicSound);
}

} // namespace tempest
