#ifndef SETTINGS_HPP
#define SETTINGS_HPP

struct Settings {
    float sfxVolume, musicVolume; //out of 1;
    float bestDistance;

    void load();
    bool save();
};

extern Settings settings;

#endif //SETTINGS_HPP
