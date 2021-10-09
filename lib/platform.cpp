#include "platform.hpp"
#include "common.hpp"
#include <algorithm>
#include <ctype.h>

static inline bool operator<(DirEnt l, DirEnt r) {
    if (l.isDir && !r.isDir) return true;
    if (!l.isDir && r.isDir) return false;
    return case_insensitive_ascii_compare(l.name, r.name) < 0;
}

#ifdef _WIN32

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// WINDOWS                                                                                                          ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <ShellScalingAPI.h>

//returns true on success
bool create_dir_if_not_exist(const char * dirpath) {
    bool succ = CreateDirectoryA(dirpath, nullptr);
    return succ || GetLastError() == ERROR_ALREADY_EXISTS;
}

List<DirEnt> fetch_dir_info_recursive(const char * dirpath) {
    char * wildcard = dsprintf(nullptr, "%s/*", dirpath);
    WIN32_FIND_DATAA findData = {};
    HANDLE handle = FindFirstFileA(wildcard, &findData);
    free(wildcard);

    if (handle == INVALID_HANDLE_VALUE) {
        int err = GetLastError();
        assert(err == ERROR_FILE_NOT_FOUND || err == ERROR_PATH_NOT_FOUND);
        return {};
    }

    List<DirEnt> dir = {};
    do {
        if (strcmp(findData.cFileName, ".") && strcmp(findData.cFileName, "..")) {
            if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                char * subdirpath = dsprintf(nullptr, "%s/%s", dirpath, findData.cFileName);
                dir.add({ dup(findData.cFileName), true, fetch_dir_info_recursive(subdirpath) });
                free(subdirpath);
            } else {
                dir.add({ dup(findData.cFileName), false });
            }
        }
    } while (FindNextFileA(handle, &findData));
    assert(GetLastError() == ERROR_NO_MORE_FILES);
    assert(FindClose(handle));

    //sort entries alphabetically
    std::sort(dir.begin(), dir.end());
    return dir;
}

void view_file_in_system_file_browser(const char * path) {
    char fullpath[MAX_PATH + 1] = {};
    if (GetFullPathNameA(path, sizeof(fullpath), fullpath, nullptr)) {
        char * cmd = dsprintf(nullptr, "%%SystemRoot%%\\Explorer.exe /select,\"%s\"", fullpath);
        system(cmd);
        free(cmd);
    }
}

void open_folder_in_system_file_browser(const char * path) {
    char * cmd = dsprintf(nullptr, "start \"\" \"%s\"", path);
    system(cmd);
    free(cmd);
}

void set_current_working_directory(const char * path) {
    assert(SetCurrentDirectory(path));
}

void handle_dpi_awareness() {
    //NOTE: SetProcessDpiAwarenessContext isn't available on targets older than win10-1703
    //      and will therefore crash at startup on those systems if we call it normally,
    //      so we load the function pointer manually to make it fail silently instead
    HMODULE user32_dll = LoadLibraryA("User32.dll");
    if (user32_dll) {
        int (WINAPI * Loaded_SetProcessDpiAwarenessContext) (long long) =
            (int (WINAPI *) (long long)) GetProcAddress(user32_dll, "SetProcessDpiAwarenessContext");
        if (Loaded_SetProcessDpiAwarenessContext) {
            if (!Loaded_SetProcessDpiAwarenessContext(-4)) { //DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
                printf("WARNING: SetProcessDpiAwarenessContext failed!\n");
            }
        } else {
            printf("WARNING: Could not load SetProcessDpiAwarenessContext!\n");

            bool should_try_setprocessdpiaware = false;
            //OK, let's try to load `SetProcessDpiAwareness` then,
            //that way we get at least *some* DPI awareness as far back as win8.1
            HMODULE shcore_dll = LoadLibraryA("Shcore.dll");
            if (shcore_dll) {
                HRESULT (WINAPI * Loaded_SetProcessDpiAwareness) (PROCESS_DPI_AWARENESS) =
                    (HRESULT (WINAPI *) (PROCESS_DPI_AWARENESS)) GetProcAddress(shcore_dll, "SetProcessDpiAwareness");
                if (Loaded_SetProcessDpiAwareness) {
                    if (Loaded_SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE) != S_OK) {
                        printf("WARNING: SetProcessDpiAwareness failed! (fallback)\n");
                    }
                } else {
                    printf("WARNING: Could not load SetProcessDpiAwareness! (fallback)\n");
                    should_try_setprocessdpiaware = true;
                }
            } else {
                printf("WARNING: Could not load Shcore.dll! (fallback)\n");
                should_try_setprocessdpiaware = true;
            }
            if (should_try_setprocessdpiaware) {
                BOOL (WINAPI * Loaded_SetProcessDPIAware)(void) =
                    (BOOL (WINAPI *) (void)) GetProcAddress(user32_dll, "SetProcessDPIAware");
                if (Loaded_SetProcessDPIAware) {
                    if (!Loaded_SetProcessDPIAware()) {
                        printf("WARNING: SetProcessDPIAware failed! (fallback 2)");
                    }
                } else {
                    printf("WARNING: Could not load SetProcessDPIAware! (fallback 2)\n");
                }
            }
            FreeLibrary(shcore_dll);
        }

        FreeLibrary(user32_dll);
    } else {
        printf("WARNING: Could not load User32.dll!\n");
    }
}

#else

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// MACOS                                                                                                            ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>
#include <unistd.h> //chdir

//returns true on success
bool create_dir_if_not_exist(const char * dirpath) {
    //TODO: what should these permissions actually be? idk how to unix lmao
    int err = mkdir(dirpath, S_IRWXU | S_IRWXG | S_IRWXO);
    return !err || errno == EEXIST;
}

List<DirEnt> fetch_dir_info_recursive(const char * dirpath) {
    //open directory
    DIR * dp = opendir(dirpath);
    assert(dp);

    //scan for level files
    List<DirEnt> dir = {};
    dirent * ep = readdir(dp);
    while (ep) {
        if (strcmp(ep->d_name, ".") && strcmp(ep->d_name, "..") && strcmp(ep->d_name, ".DS_Store")) {
            if (ep->d_type == DT_DIR) {
                char * subdirpath = dsprintf(nullptr, "%s/%s", dirpath, ep->d_name);
                dir.add({ dup(ep->d_name), true, fetch_dir_info_recursive(subdirpath) });
                free(subdirpath);
            } else if (ep->d_type == DT_REG) {
                dir.add({ dup(ep->d_name), false });
            }
        }
        ep = readdir(dp);
    }
    closedir(dp);

    //sort entries alphabetically
    std::sort(dir.begin(), dir.end());
    return dir;
}

void view_file_in_system_file_browser(const char * path) {
    char * cmd = dsprintf(nullptr, "open -R \"%s\"", path);
    system(cmd);
    free(cmd);
}

void open_folder_in_system_file_browser(const char * path) {
    char * cmd = dsprintf(nullptr, "open \"%s\"", path);
    system(cmd);
    free(cmd);
}

void set_current_working_directory(const char * path) {
    assert(!chdir(path));
}

#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/// UNIVERSAL                                                                                                        ///
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//NOTE: I'm surprised that this works on windows, it looks like a unix thing - is it a clang extension or something?
#include <sys/stat.h>
#if _WIN32
#define stat _stat
#endif
bool file_exists(const char * filepath) {
    struct stat s;
    return stat(filepath, &s) == 0 && (s.st_mode & S_IFMT) != S_IFDIR;
}
bool is_directory(const char * filepath) {
    struct stat s;
    return stat(filepath, &s) == 0 && (s.st_mode & S_IFMT) == S_IFDIR;
}

//NOTE: the correct behavior of this function is unfortunately not guaranteed by the standard
char * read_entire_file(const char * filepath, long *fileLength) {
    FILE * f = fopen(filepath, "rb");
    assert(f);
    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);  //same as rewind(f);

    char * string = (char *) malloc(fsize + 1);
    fread(string, fsize, 1, f);
    fclose(f);

    string[fsize] = 0;

    if (fileLength) *fileLength = fsize;

    return string;
}

bool write_entire_file(const char * filepath, const char * string, int len) {
    assert(filepath && string);

    //parse path string into its individual components
    char * path = dup(filepath);
    List<char *> dirs = {};
    char * dir = strtok(path, "\\/");
    while (dir) {
        dirs.add(dir);
        dir = strtok(nullptr, "\\/");
    }

    //create all necessary intermediate directories
    char * rebuild = dup(dirs[0]);
    for (int i = 0; i < dirs.len - 1; ++i) {
        //TODO: what should these permissions actually be? idk how to unix lmao
        if (!create_dir_if_not_exist(rebuild)) {
            dirs.finalize();
            free(path);
            free(rebuild);
            return false;
        }
        rebuild = dsprintf(rebuild, "/%s", dirs[i + 1]);
    }
    dirs.finalize();
    free(path);

    //finally, write the file
    FILE * f = fopen(rebuild, "wb");
    free(rebuild);
    if (!f) return false;
    if (len < 0) len = strlen(string);
    if (!fwrite(string, len, 1, f)) return false;
    if (fclose(f)) return false;
    return true;
}

static void recursive_flatten(List<char *> & out, List<DirEnt> dir, const char * path) {
    for (DirEnt ent : dir) {
        if (ent.isDir) {
            char * subpath = dsprintf(nullptr, "%s%s%s", path, is_empty(path)? "" : "/", ent.name);
            recursive_flatten(out, ent.children, subpath);
            free(subpath);
        } else {
            out.add(dsprintf(nullptr, "%s%s%s", path, is_empty(path)? "" : "/", ent.name));
        }
    }
}

List<char *> flatten_dir_to_path_list(List<DirEnt> dir) {
    List<char *> ret = {};
    recursive_flatten(ret, dir, "");
    return ret;
}


void deep_finalize(List<DirEnt> & dir) {
    for (DirEnt & entry : dir) {
        free(entry.name);
        if (entry.isDir) {
            deep_finalize(entry.children);
        }
    }
    dir.finalize();
}
