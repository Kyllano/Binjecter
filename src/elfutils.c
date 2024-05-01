#include "binjecter.h"

extern struct arguments arguments;
extern Elf64_Ehdr* exec_header;

bfd* open_elf_bfd(void) {
    bfd_init(); //init framework

    //opening the ELF file and putting it in a pointer to Binary File Descriptor
    bfd* pbfd = bfd_openr(arguments.elf_path, NULL);
    if (pbfd == NULL) {
        fprintf(stderr, "Error opening ELF file\n");
        exit(ERROR_OPENING_ELF);
    }

    //Check if it actually is an elf
    if (!bfd_check_format(pbfd, bfd_object)) {
        fprintf(stderr, "Provided file is not an ELF file\n");
        bfd_close(pbfd);
        exit(ERROR_FORMAT);
    }

    //Check if it is executable and 64 bits arch
    const bfd_arch_info_type* arch_info = bfd_get_arch_info(pbfd);
    if (arch_info->bits_per_address != 64) {
        fprintf(stderr, "Provided file is not 64 bit architecture\n");
        bfd_close(pbfd);
        exit(ERROR_FORMAT);
    }

    //Check if it is executable
    if ((pbfd->flags & EXEC_P) != EXEC_P) {
        fprintf(stderr, "Provided file is not executable\n");
        bfd_close(pbfd);
        exit(ERROR_FORMAT);
    }

    return pbfd;
}

void parse_exec_header(void) {
    if (exec_header != NULL) {
        return;
    }

    exec_header = (Elf64_Ehdr*)malloc(1 * sizeof(Elf64_Ehdr));

    FILE* elf = fopen(arguments.elf_path, "rb");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }
    if (fseek(elf, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fread(exec_header, sizeof(Elf64_Ehdr), 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    fclose(elf);
    return;
}

int find_pt_note(void) {
    parse_exec_header();
    FILE* elf = fopen(arguments.elf_path, "rb");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }
    if (fseek(elf, exec_header->e_phoff, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Phdr program_header;
    if (fread(&program_header, exec_header->e_phentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    size_t phi = 0;
    while ((program_header.p_type != PT_NOTE) && phi < exec_header->e_phnum) {
        if (fread(&program_header, exec_header->e_phentsize, 1, elf) != 1) {
            fprintf(stderr, "Cannot read into file\n");
            fclose(elf);
            exit(ERROR_FILE);
        }
        phi++;
    }

    fclose(elf);

    if (phi >= exec_header->e_phnum) {
        return -1;
    }
    return phi;
}

void overwrite_ph_header(size_t header_num, size_t section_size, size_t section_offset) {
    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    if (fseek(elf, exec_header->e_phoff + (header_num * exec_header->e_phentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Phdr program_header;
    if (fread(&program_header, exec_header->e_phentsize, 1, elf) != 1) {

        fprintf(stderr, "Cannot read into file COUCOU\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    program_header.p_type = PT_LOAD;
    program_header.p_paddr = arguments.base_addr;
    program_header.p_vaddr = arguments.base_addr;
    program_header.p_filesz = section_size;
    program_header.p_memsz = section_size;
    program_header.p_flags = PF_X | PF_R;
    program_header.p_align = 0x1000;
    program_header.p_offset = section_offset;

    if (fseek(elf, exec_header->e_phoff + (header_num * exec_header->e_phentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fwrite(&program_header, exec_header->e_phentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot write into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    printf("We done changin the program header <3\n");

    fclose(elf);
}

sh_info add_section(void) {
    //info for later
    sh_info sh_infos;

    FILE* elf = fopen(arguments.elf_path, "ab");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    if (fseek(elf, 0, SEEK_END) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    if ((sh_infos.offset = ftell(elf)) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    FILE* payload = fopen(arguments.payload_path, "rb");
    if (payload == NULL) {
        fprintf(stderr, "Cannot open payload file\n");
        exit(ERROR_FILE);
    }

    //Read and rewrite 4096 bit every iteration
    char read_buffer[4096];
    size_t nb_read = 1;
    size_t nb_written = 0;
    while (!feof(payload)) {
        nb_read = fread(read_buffer, 1, 4096, payload);
        if (fwrite(read_buffer, 1, nb_read, elf) != nb_read) {
            fprintf(stderr, "Cannot write into file\n");
            fclose(elf);
            fclose(payload);
            exit(ERROR_FILE);
        }
        nb_written += nb_read;
    }
    sh_infos.size = nb_written;

    //For completeness sake, I put the address in the sh_info, so that it is easier to retrieve in overwrite_hs
    sh_infos.base_addr = arguments.base_addr;

    fclose(elf);
    fclose(payload);

    return sh_infos;
}

void sanitize_base_addr(long int offset, uint64_t* base_addr) {
    *base_addr += (labs((offset - (long int)(*base_addr))) % 4096);
}

Elf64_Word find_section_index(char* section_name_to_replace) {
    //Get le exec header et ouvre le file stp
    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    //Go lire le shstrtab section header
    if (fseek(elf, exec_header->e_shoff + (exec_header->e_shstrndx * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Shdr shstrtab_header;
    if (fread(&shstrtab_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //Maintenant go lire la section shstrtab
    if (fseek(elf, shstrtab_header.sh_offset, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    char section_names[shstrtab_header.sh_size];
    if (fread(section_names, 1, shstrtab_header.sh_size, elf) != shstrtab_header.sh_size) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //Maintenant trouver l'index de la section
    Elf64_Word sh_index;
    for (size_t i = 0; i <= exec_header->e_shnum; i++) {
        if (i == exec_header->e_shnum) {
            fprintf(stderr, "Counld not find the section header for \"%s\"\n", section_name_to_replace);
            fclose(elf);
            exit(ERROR_FILE);
        }

        if (fseek(elf, exec_header->e_shoff + (i * exec_header->e_shentsize), SEEK_SET) == -1) {
            fprintf(stderr, "Cannot seek into file\n");
            fclose(elf);
            exit(ERROR_FILE);
        }
        Elf64_Shdr section_header;
        if (fread(&section_header, exec_header->e_shentsize, 1, elf) != 1) {
            fprintf(stderr, "Cannot read into file\n");
            fclose(elf);
            exit(ERROR_FILE);
        }

        //Check si c est la bonne
        if (strcmp(section_names + section_header.sh_name, section_name_to_replace) == 0) {
            sh_index = i;
            break;
        }
    }
    fclose(elf);
    return sh_index;
}

void overwrite_sh(char* section_name_to_replace, sh_info sh_infos) {
    //Get le exec header et ouvre le file stp

    //Trouve l'index del a section
    Elf64_Word sh_index = find_section_index(section_name_to_replace);

    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    //On modifie le header
    if (fseek(elf, exec_header->e_shoff + (sh_index * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Shdr sh_header;
    if (fread(&sh_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //Now to modify some shit
    sh_header.sh_type = SHT_PROGBITS;
    sh_header.sh_offset = sh_infos.offset;
    sh_header.sh_addr = sh_infos.base_addr;
    sh_header.sh_size = sh_infos.size;
    sh_header.sh_addralign = 16;
    sh_header.sh_flags = SHF_EXECINSTR;

    //We're now ready to rewrite the section header
    if (fseek(elf, exec_header->e_shoff + (sh_index * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fwrite(&sh_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot write into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    printf("section %s overwritten in elf :)\n", section_name_to_replace);
    fclose(elf);
}

void reorder_sections(void) {
    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    //Je pense le moins relou, c est de lire toutes les sections, les foutres dans un tableau
    //Tout reordonner, ensuite modifier les bits link et enfin tout réécrire. Go faire ca

    if (fseek(elf, exec_header->e_shoff, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Shdr* sh_headers = (Elf64_Shdr*)malloc(exec_header->e_shnum * exec_header->e_shentsize);
    if (fread(sh_headers, exec_header->e_shentsize, exec_header->e_shnum, elf) != exec_header->e_shnum) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    if (fseek(elf, exec_header->e_shoff, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fwrite(sh_headers, exec_header->e_shentsize, exec_header->e_shnum, elf) != exec_header->e_shnum) {
        fprintf(stderr, "Cannot write into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //Also on a un tableau d'int, qui pour chaque section donnera la pierre de rosette
    //pour les changements des champs sh_link (vu que a bouger les sections, il pourront etre modifiés).
    //On se balladera avec un tableau qui contient la propriété suivante :
    // tab[ancien_sh_link] = nvx_sh_link

    Elf64_Word sh_links[exec_header->e_shnum];
    for (size_t i = 0; i < exec_header->e_shnum; i++) {
        sh_links[i] = 0;
    }
    for (size_t i = 0; i < exec_header->e_shnum; i++) {
        //D abord on recupere les sh_links et on changera la pierre quand on swappera nos sections
        if (sh_headers[i].sh_link != 0) {
            sh_links[sh_headers[i].sh_link] = sh_headers[i].sh_link;
        }
    }

    //Ici, sachant que .note.ABI-tag est en
    for (Elf64_Half i = 0; i < exec_header->e_shnum - 1; i++) {
        if (sh_headers[i].sh_offset > sh_headers[i + 1].sh_offset) {
            //Swap em sections!!
            Elf64_Shdr tmp_shdr = sh_headers[i + 1];
            sh_headers[i + 1] = sh_headers[i];
            sh_headers[i] = tmp_shdr;

            if (sh_links[i] != 0) {
                sh_links[i]--;
            }

            if (i == (exec_header->e_shstrndx - 1)) {
                exec_header->e_shstrndx--;
            }
        }
    }

    //Et maintenant on fait une passe dans les sections pour update les links
    for (size_t i = 0; i < exec_header->e_shnum; i++) {
        Elf64_Word new_index = sh_links[sh_headers[i].sh_link];
        //printf("%ld : avait %d, maintenant %d\n", i, sh_headers[i].sh_link, new_index);
        sh_headers[i].sh_link = new_index;
    }

    if (fseek(elf, exec_header->e_shoff, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fwrite(sh_headers, exec_header->e_shentsize, exec_header->e_shnum, elf) != exec_header->e_shnum) {
        fprintf(stderr, "Cannot write into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //rewrite shstrndx
    if (fseek(elf, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fwrite(exec_header, exec_header->e_ehsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot write into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    printf("Reordered sections :D\n");

    //free(sh_headers);
    fclose(elf);
}

void change_section_name(char* old_name, char* new_name) {
    if (strlen(new_name) > strlen(old_name)) {
        fprintf(stderr, "New name of section must be shorter or equal to old name of section\n");
        exit(ERROR_ARGS);
    }

    //Get le exec header et ouvre le file stp
    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    //Go lire le shstrtab section header
    if (fseek(elf, exec_header->e_shoff + (exec_header->e_shstrndx * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Shdr shstrtab_header;
    if (fread(&shstrtab_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //Maintenant go lire la section shstrtab
    if (fseek(elf, shstrtab_header.sh_offset, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    char section_names[shstrtab_header.sh_size];
    if (fread(section_names, 1, shstrtab_header.sh_size, elf) != shstrtab_header.sh_size) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //Maintenant trouver le nom de la section qu on remodifie
    for (Elf64_Xword i = 0; i < shstrtab_header.sh_size; i++) {
        if (strcmp(section_names + i, old_name) == 0) {
            printf("on modifie un nom :3\n");
            //strcpy(section_names + i, new_name);
            for (size_t j = 0; j < strlen(new_name); j++) {
                section_names[i+j] = new_name[j];
            }
            section_names[i+strlen(new_name)] = '\0';
        }
    }

    if (fseek(elf, shstrtab_header.sh_offset, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fwrite(section_names, 1, shstrtab_header.sh_size, elf) != shstrtab_header.sh_size) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    fclose(elf);
}

void change_entrypoint(void) {
    //Get le exec header et ouvre le file stp
    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    exec_header->e_entry = arguments.base_addr;

    if (fseek(elf, 0, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fwrite(exec_header, exec_header->e_ehsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    printf("entrypoint rewritten\n");

    fclose(elf);
}

size_t find_func_in_dynsym(char* func_name) {
    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    //D ABORD, on get l index de la fonction a chercher dans dynstr
    Elf64_Word sh_index = find_section_index(".dynstr");
    if (fseek(elf, exec_header->e_shoff + (sh_index * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Shdr sh_header;
    if (fread(&sh_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    if (fseek(elf, sh_header.sh_offset, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    char section_dynstr[sh_header.sh_size];
    if (fread(section_dynstr, sizeof(char), sh_header.sh_size, elf) != sh_header.sh_size) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    size_t fun_num = 0;
    for (size_t i = 0; i < sh_header.sh_size; i++) {
        if (strcmp(section_dynstr + i, func_name) == 0) {
            fun_num = i;
            break;
        }
    }

    //ENSUITE, go prendre l index dans dynsym
    sh_index = find_section_index(".dynsym");
    if (fseek(elf, exec_header->e_shoff + (sh_index * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fread(&sh_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    if (fseek(elf, sh_header.sh_offset, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    //printf("On a size %ld / symsize %ld = %ld", sh_header.sh_size, sizeof(Elf64_Sym), sh_header.sh_size/sizeof(Elf64_Sym)); //C est good
    Elf64_Sym section_dynsym[sh_header.sh_size / sizeof(Elf64_Sym)];
    if (fread(section_dynsym, sizeof(Elf64_Sym), sh_header.sh_size / sizeof(Elf64_Sym), elf) != sh_header.sh_size / sizeof(Elf64_Sym)) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    //Je vais recycler fun_num
    for (size_t i = 0; i < sh_header.sh_size / sizeof(Elf64_Sym); i++) {
        if (section_dynsym[i].st_name == fun_num) {
            fun_num = i;
        }
    }

    fclose(elf);

    return fun_num;
}

void replace_func_address(size_t func_index_dynsym) {
    FILE* elf = fopen(arguments.elf_path, "rb+");
    if (elf == NULL) {
        fprintf(stderr, "Cannot open elf file\n");
        exit(ERROR_FILE);
    }

    //D ABORD, on get l index de la fonction a chercher dans .rela.plt
    Elf64_Word sh_index = find_section_index(".rela.plt");
    if (fseek(elf, exec_header->e_shoff + (sh_index * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Shdr sh_header;
    if (fread(&sh_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    if (fseek(elf, sh_header.sh_offset, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    Elf64_Rela section_rela_plt[sh_header.sh_size / sizeof(Elf64_Rela)];
    if (fread(section_rela_plt, sizeof(Elf64_Rela), sh_header.sh_size / sizeof(Elf64_Rela), elf) != sh_header.sh_size / sizeof(Elf64_Rela)) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    size_t fun_num = 0;
    for (size_t i = 0; i < sh_header.sh_size / sizeof(Elf64_Rela); i++) {
        if ((section_rela_plt[i].r_info >> 32) == func_index_dynsym) {
            fun_num = i;
            break;
        }
    }

    //ENSUITE on replace l'addresse de la fonction a l indice trouvé dans .got.plt
    sh_index = find_section_index(".got.plt");
    if (fseek(elf, exec_header->e_shoff + (sh_index * exec_header->e_shentsize), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    if (fread(&sh_header, exec_header->e_shentsize, 1, elf) != 1) {
        fprintf(stderr, "Cannot read into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    if (fseek(elf, sh_header.sh_offset, SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    // On ajoute +3 car la premiere entree de la got est pour le linker et les 2 autres sont à 0 parce en fait
    if (fseek(elf, sh_header.sh_offset + (sizeof(uint64_t) * (fun_num + 3)), SEEK_SET) == -1) {
        fprintf(stderr, "Cannot seek into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }
    uint64_t address = arguments.base_addr;
    if (fwrite(&address, sizeof(uint64_t), 1, elf) != 1) {
        fprintf(stderr, "Cannot write into file\n");
        fclose(elf);
        exit(ERROR_FILE);
    }

    printf("got sucessfully injected :0\n");
    fclose(elf);
}