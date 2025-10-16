#ifndef __UTILS_ARGUMENT_
#define __UTILS_ARGUMENT_

void argument_get(const char** rom_path, const char** bios_path, const char** force_core);
void argument_get_default_bios(char* path, const char* core_name);
void argument_set_default_bios(const char* path, const char* core_name);

#endif