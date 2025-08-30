#include <stdio.h>
#include <string.h>

const char base_path[] = "include/cores/nes/mappers";
char filename[1 << 16];

int main(){
    freopen("include/cores/nes/mappers.h", "w", stdout);
    
    printf(
        "#ifndef __MAPPERS_H__\n"
        "#define __MAPPERS_H__\n\n"
    );

    for(int i = 0; i < 256; i++){
        sprintf(filename, "%s/mapper%d.h", base_path, i);
        FILE* fptr = fopen(filename, "r");
        if(fptr){
            fclose(fptr);
            printf("#include \"cores/nes/mappers/mapper%d.h\"\n", i);
        }
    }

    printf("\n");

    printf(
        "typedef struct mapper_t {\n"
        "\tchar  supported;\n"
        "\tvoid* mapper_init;\n"
        "\tvoid* cpu_read;\n"
        "\tvoid* cpu_write;\n"
        "\tvoid* ppu_read;\n"
        "\tvoid* ppu_write;\n"
        "} mapper_t;\n\n"
    );

    printf("static mapper_t mappers[256] = {\n");
    for(int i = 0; i < 256; i++){
        sprintf(filename, "%s/mapper%d.h", base_path, i);
        FILE* fptr = fopen(filename, "r");
        if(fptr){
            fclose(fptr);
            printf("\t[%d] = {1, mapper%d_init, mapper%d_cpu_read, mapper%d_cpu_write, mapper%d_ppu_read, mapper%d_ppu_write},\n", i, i, i, i, i, i);
        }
    }
    printf("};\n\n");

    printf("#endif\n");
}