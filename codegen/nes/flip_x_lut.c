#include <stdio.h>

int main(){
    for(int i = 0; i < 256; i++){
        int out = 0;
        for(int j = 0; j < 8; j++){
            int bit = !!(i & (1 << (7-j)));
            out |= bit << j;
        }
        printf("0x%02X, ", out);
    }
}