#include <stdio.h>

int main(){
    printf("static float pulse_table[31] = { ");
    for(int i = 0; i < 31; i++){
        float val = 95.52 / (8128.0 / i + 100.0);
        printf("%f, ", val);
    }
    printf("};\n");
    
    printf("static float tnd_table[203] = { ");
    for(int i = 0; i < 203; i++){
        float val = 163.67 / (24329.0 / i + 100.0);
        printf("%f, ", val);
    }
    printf("};\n");
}