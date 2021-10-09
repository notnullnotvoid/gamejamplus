#ifndef SETTINGS_HPP
#define SETTINGS_HPP

struct Settings {
    float sfxVolume, musicVolume; //out of 1;

    void load();
    bool save();
};

extern Settings settings;

#endif //SETTINGS_HPP
