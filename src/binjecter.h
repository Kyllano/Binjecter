#ifndef __BINJECTER_H__
#define __BINJECTER_H__

/*------------------------------------------------------------
  INCLUDES & DEFINES & ERROR CODES
------------------------------------------------------------*/
#include <stdbool.h>
#include <stdio.h>
#include <argp.h>
#include <bfd.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>
#include <elf.h>
#include <math.h>
#include <string.h>

#define NO_ERROR                    0
#define ERROR_UNKNOWN               1
#define ERROR_OPENING_ELF           2
#define ERROR_FORMAT                3
#define ERROR_ARGS                  4
#define ERROR_FILE                  5

/*------------------------------------------------------------
  STRUCTS & FUNCTIONS FOR ARGUMENTS
------------------------------------------------------------*/

struct arguments {
    char * elf_path;
    char * payload_path;
    char * section_name;
    uint64_t base_addr;
    bool change_entry_function;
};
void parse_all(int argc, char** argv, struct arguments* arguments);
typedef struct sh_info_t {
    size_t size;
    long int offset;
    uint64_t base_addr;
} sh_info;

/*------------------------------------------------------------
  ELFUTILS.C FUNCTIONS
------------------------------------------------------------*/

bfd* open_elf_bfd(void);
int find_pt_note(void);
void overwrite_ph_header(size_t header_num, size_t section_size, size_t section_offset);
sh_info add_section(void);
void sanitize_base_addr(long int offset, uint64_t* base_addr);
Elf64_Word find_section_index(char* section_name_to_replace);
void overwrite_sh(char* section_name_to_replace, sh_info sh_infos);
void reorder_sections(void);
void change_section_name(char* old_name, char* new_name);
void change_entrypoint(void);
size_t find_func_in_dynsym(char* func_name);
void replace_func_address(size_t func_index_dynsym);

#endif /*__BINJECTER_H__*/