#include "AudioManager.h"
#include "Logger.h"

AudioManager& AudioManager::Get() {
    static AudioManager instance;
    return instance;
}

AudioManager::~AudioManager() {
    Deinit();
}

void AudioManager::Init() {
    m_soloud.init();
    m_soloud.set3dListenerParameters(0, 0, 0, 0, 0, 1, 0, 1, 0);
    Logger::Log("AudioManager initialized");
}

void AudioManager::Deinit() {
    StopAll();
    for (auto& pair : m_sounds) {
        delete pair.second;
    }
    m_sounds.clear();
    m_soloud.deinit();
    Logger::Log("AudioManager deinitialized");
}

bool AudioManager::LoadSound(const std::string& name, const std::string& path) {
    if (m_sounds.find(name) != m_sounds.end()) return true;

    SoLoud::Wav* wav = new SoLoud::Wav();
    if (wav->load(path.c_str()) != SoLoud::SO_NO_ERROR) {
        Logger::Log("Failed to load sound: " + path);
        delete wav;
        return false;
    }

    m_sounds[name] = wav;
    return true;
}

unsigned int AudioManager::Play2D(const std::string& name, float volume, bool loop) {
    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) return 0;

    it->second->setLooping(loop);
    unsigned int handle = m_soloud.play(*it->second, volume);
    return handle;
}

unsigned int AudioManager::Play3D(const std::string& name, float x, float y, float z, float volume, bool loop) {
    auto it = m_sounds.find(name);
    if (it == m_sounds.end()) return 0;

    it->second->setLooping(loop);
    unsigned int handle = m_soloud.play3d(*it->second, x, y, z, 0, 0, 0, volume);
    return handle;
}

void AudioManager::UpdateListener(float x, float y, float z, float lookX, float lookY, float lookZ, float upX, float upY, float upZ) {
    m_soloud.set3dListenerParameters(x, y, z, lookX, lookY, lookZ, upX, upY, upZ);
    m_soloud.update3dAudio();
}

void AudioManager::Update3DSound(unsigned int handle, float x, float y, float z) {
    if (m_soloud.isValidVoiceHandle(handle)) {
        m_soloud.set3dSourceParameters(handle, x, y, z);
    }
}

void AudioManager::SetVolume(unsigned int handle, float volume) {
    m_soloud.setVolume(handle, volume);
}

void AudioManager::Stop(unsigned int handle) {
    m_soloud.stop(handle);
}

void AudioManager::StopAll() {
    m_soloud.stopAll();
}
