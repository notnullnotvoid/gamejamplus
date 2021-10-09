//this program walks various directories of the project to generate a .ninja file
//yes this is a pun on "bob the builder" don't @ me

//bob/build.sh (or bob/build.bat for windows) builds bob.
//you only need to build bob once before you can build the rest of the project
//(but it's incremental and fast so we run it every build anyway for simplicity)

#define USE_LLD_DIRECTLY 0

#if USE_LLD_DIRECTLY
//for invoking lld directly
#define WIN32_LEAN_AND_MEAN
#include "microsoft_craziness.h"
#include <wchar.h>

static inline char * utf16_to_utf8(wchar_t * src) {
    int size = WideCharToMultiByte(CP_UTF8, 0, src, -1, NULL, 0, NULL, NULL); //get required size
    char * dst = (char *) malloc(size);
    assert(WideCharToMultiByte(CP_UTF8, 0, src, -1, dst, size, NULL, NULL)); //actually write string
    return dst;
}
#endif

#include "common.hpp"
#include "types.hpp"
#include "platform.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <initializer_list>


static bool verbose;

template <typename VAL, typename KEY>
static inline VAL match_pair(std::initializer_list<Pair<VAL, KEY>> pairs, KEY key, VAL defaultVal) {
    for (auto it = pairs.begin(); it != pairs.end(); ++it) {
        if (it->second == key) {
            return it->first;
        }
    }
    return defaultVal;
}

template <typename TYPE>
static inline bool one_of(std::initializer_list<TYPE> list, TYPE key) {
    for (auto it = list.begin(); it != list.end(); ++it) {
        if (*it == key) {
            return true;
        }
    }
    return false;
}

static inline bool ends_with(const char * str, const char * end) {
    int slen = strlen(str), elen = strlen(end);
    if (elen > slen) return false;
    return !strcmp(str + (slen - elen), end);
}

////////////////////////////////////////////////////////////////////////////////
/// DIRECTORY WALKING                                                        ///
////////////////////////////////////////////////////////////////////////////////

enum FileType {
    FILE_NONE,
    FILE_CPPLIB,
    FILE_CLIB,
    FILE_SRC,
};

struct File {
    FileType type;
    char * dir;
    char * name;
    char * ext;
};

static void add_files(List<File> & files, FileType type, const char * dirpath, const char * ext) {
    List<DirEnt> dir = fetch_dir_info_recursive(dirpath);
    List<char *> paths = flatten_dir_to_path_list(dir);
    for (char * path : paths) {
        if (ends_with(path, ext)) {
            files.add({ type, dup(dirpath), dup(path, strlen(path) - strlen(ext)), dup(ext) });
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
/// MAIN                                                                     ///
////////////////////////////////////////////////////////////////////////////////

#ifdef _WIN32
    static const bool windows = true;
#else
    static const bool windows = false;
#endif

int main(int argc, char ** argv) {
    const char * root = ".";
    const char * output = "build.ninja";
    const char * config = "";

    //parse command line arguments
    enum ArgType { ARG_NONE, ARG_ROOT, ARG_OUTPUT, ARG_CONFIG };
    ArgType type = ARG_NONE;
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "-v")) {
            verbose = true;
        } else if (!strcmp(argv[i], "-r")) {
            type = ARG_ROOT;
        } else if (!strcmp(argv[i], "-o")) {
            type = ARG_OUTPUT;
        } else if (!strcmp(argv[i], "-c")) {
            type = ARG_CONFIG;
        } else if (type == ARG_ROOT) {
            root = argv[i];
            type = ARG_NONE;
        } else if (type == ARG_OUTPUT) {
            output = argv[i];
            type = ARG_NONE;
        } else if (type == ARG_CONFIG) {
            config = argv[i];
            type = ARG_NONE;
        }
    }

    //match config name strings
    enum Config { CONFIG_DEBUG, CONFIG_DEV, CONFIG_PERF, CONFIG_RELEASE };
    const char * configNames[4] = { "debug", "dev", "perf", "release" };
    Config conf = CONFIG_DEV; //default if no such argument is provided
    for (int i = 0; i < ARR_SIZE(configNames); ++i) {
        if (!strcmp(config, configNames[i])) { //TODO: make case-insensitive?
            conf = (Config) i;
            break;
        }
    }

    const char * configEnumNames[] = { "CONFIG_DEBUG", "CONFIG_DEV", "CONFIG_PERF", "CONFIG_RELEASE" };
    if (verbose) printf("selected config: %s\n", configEnumNames[conf]);
    set_current_working_directory(root);

    //write header and rules
    FILE * out = fopen(output, "w");
    assert(out);
    //NOTE: dev and perf sharing a directory here is intentional
    const char * buildSubDir = conf == CONFIG_DEBUG? "debug" : conf == CONFIG_RELEASE? "release" : "dev";
    fprintf(out, "ninja_required_version = 1.7\nbuilddir = build/%s/%s\n\n", windows? "win" : "mac", buildSubDir);

    const char * debugArgs = conf == CONFIG_RELEASE? " -DCONFIG_RELEASE=1 -DNDEBUG " : " -DDEBUG ";
    const char * common = " clang -MMD -MF $out.d -c $in -o $out -fno-exceptions -fno-rtti "
        " -msse3 -fno-strict-aliasing -I. -Ilib -Isoloud -ISDL2 -Iimgui -D_CRT_SECURE_NO_WARNINGS -g -gfull ";
    const char * sanArgs = conf == CONFIG_DEBUG?
        " -fsanitize=address    -fsanitize=undefined -fsanitize-address-use-after-scope " : "";

    fprintf(out, "rule lib\n");
    fprintf(out, "    command = %s -std=c++17 %s%s%s\n", common, debugArgs, sanArgs,
        conf == CONFIG_DEBUG? "-O0 -ftrapv " : " -Os -fwrapv");
    fprintf(out, "    description = Build $in\n");
    fprintf(out, "    depfile = $out.d\n");

    fprintf(out, "rule clib\n");
    fprintf(out, "    command = %s%s%s%s\n", common, debugArgs, sanArgs,
        conf == CONFIG_DEBUG? "-O0 -ftrapv " : " -Os -fwrapv");
    fprintf(out, "    description = Build $in\n");
    fprintf(out, "    depfile = $out.d\n");

    fprintf(out, "rule src\n");
    fprintf(out, "    command = %s -std=c++17 -Wall -Wno-absolute-value -Wno-invalid-offsetof %s%s%s\n",
        common, debugArgs, sanArgs, conf == CONFIG_DEBUG || conf == CONFIG_DEV? "-O0 -ftrapv " : "-Os -fwrapv ");
    fprintf(out, "    description = Build $in\n");
    fprintf(out, "    depfile = $out.d\n");

    fprintf(out, "rule link\n");
    if (windows) {
        #if USE_LLD_DIRECTLY
        Find_Result result = find_visual_studio_and_windows_sdk();
        printf("windows_sdk_root: %s\n", utf16_to_utf8(result.windows_sdk_root));
        printf("windows_sdk_um_library_path: %s\n", utf16_to_utf8(result.windows_sdk_um_library_path));
        printf("windows_sdk_ucrt_library_path: %s\n", utf16_to_utf8(result.windows_sdk_ucrt_library_path));
        printf("vs_exe_path: %s\n", utf16_to_utf8(result.vs_exe_path));
        printf("vs_library_path: %s\n", utf16_to_utf8(result.vs_library_path));
        fprintf(out, "    command = lld-link.exe -debug:full $in -out:$out link/SDL2-2.0.16-x86_64-windows-gnu.lib "
            " -defaultlib:libcmt -defaultlib:oldnames -nologo \"-libpath:%s\" \"-libpath:%s\" \"-libpath:%s\" %s "
            " gdi32.lib winmm.lib ole32.lib oleaut32.lib shell32.lib version.lib advapi32.lib setupapi.lib "
            " uuid.lib user32.lib kernel32.lib imm32.lib libcpmt.lib "
            "\n", utf16_to_utf8(result.windows_sdk_um_library_path), utf16_to_utf8(result.windows_sdk_ucrt_library_path),
            utf16_to_utf8(result.vs_library_path), sanArgs);
        #else
        fprintf(out, "    command = clang -fuse-ld=lld $in -o $out link/SDL2-2.0.16-x86_64-windows-gnu.lib "
            " -lgdi32 -lwinmm -lole32 -loleaut32 -lshell32 -lversion -ladvapi32 -lsetupapi -g -gfull %s\n", sanArgs);
        #endif
    } else {
        fprintf(out, "    command = clang $in -o $out -lstdc++ link/libSDL2-2.0.16.a -framework CoreAudio "
            " -framework AudioToolbox -framework CoreVideo -framework ForceFeedback -framework IOKit -framework Carbon "
            " -framework Metal -framework AppKit -liconv -g -gfull %s\n", sanArgs);
    }
    fprintf(out, "    description = Link $out\n");
    fprintf(out, "\n");

    //gather and classify files
    List<File> files = {};
    add_files(files, FILE_CPPLIB, "soloud/src", ".cpp");
    add_files(files, FILE_CLIB, "soloud/src", ".c");
    add_files(files, FILE_CPPLIB, "imgui", ".cpp");
    add_files(files, FILE_CPPLIB, "lib", ".cpp");
    add_files(files, FILE_CLIB, "lib", ".c");
    add_files(files, FILE_SRC, "src", ".cpp");

    //write compile commands
    for (File f : files) {
        const char * rule = match_pair<const char *>({
            { "lib", FILE_CPPLIB },
            { "clib", FILE_CLIB },
            { "src", FILE_SRC },
        }, f.type, nullptr);

        if (rule) {
            fprintf(out, "build $builddir/%s.o: %s %s/%s%s\n", f.name, rule, f.dir, f.name, f.ext);
        }
    }

    //write link command
    fprintf(out, "\nbuild %s: link", windows? "game.exe" : "game");
    for (File f : files) {
        if (one_of({ FILE_CPPLIB, FILE_CLIB, FILE_SRC }, f.type)) {
            fprintf(out, " $builddir/%s.o", f.name);
        }
    }
    fprintf(out, "\n\n");
    fclose(out);

    return 0;
}
