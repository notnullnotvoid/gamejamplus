#ifndef PLATFORM_HPP
#define PLATFORM_HPP

#include "list.hpp"

struct DirEnt {
	char * name;
	bool isDir;
	List<DirEnt> children; //only for directories
};

List<DirEnt> fetch_dir_info_recursive(const char * dirpath);
List<char *> flatten_dir_to_path_list(List<DirEnt> dir);
void deep_finalize(List<DirEnt> & dir);

bool file_exists(const char * filepath);
bool is_directory(const char * filepath);
char * read_entire_file(const char * filepath, long * fileLength = nullptr);
bool write_entire_file(const char * filepath, const char * string, int len = -1); //TODO: semi-atomic version of this?
void view_file_in_system_file_browser(const char * path);
void open_folder_in_system_file_browser(const char * path);
void set_current_working_directory(const char * path);
bool create_dir_if_not_exist(const char * dirpath);

#ifdef _WIN32
void handle_dpi_awareness();
#endif //_WIN32

#endif // PLATFORM_HPP
