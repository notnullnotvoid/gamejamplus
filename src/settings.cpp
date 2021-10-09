#include "settings.hpp"
#include "common.hpp"
#include "platform.hpp"
#include "trace.hpp"

static const char * path = "settings.txt";
Settings settings;

void Settings::load() { TimeFunc
    *this = {};
    sfxVolume = 1;
    musicVolume = 0.5f;

    if (file_exists(path)) {
        char * raw = read_entire_file(path);
        List<char *> lines = split_lines_in_place(raw);
        for (char * line : lines) {
            char * token = strtok(line, " \t");
            if (!strcmp(token, "sfx")) {
                sfxVolume = atof(strtok(nullptr, " \t"));
            } else if (!strcmp(token, "music")) {
                musicVolume = atof(strtok(nullptr, " \t"));
            }
        }
        free(raw);
    }
}

bool Settings::save() { TimeFunc
    //TODO: save this semi-atomically
    FILE * f = fopen(path, "wb");
    if (!f) return false;
    fprintf(f, "sfx %f\n", sfxVolume);
    fprintf(f, "music %f\n", musicVolume);
    if (fclose(f) != 0) return false;
    return true;
}
