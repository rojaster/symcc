#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
    char buf[256];
    FILE *fd = fopen(argv[1], 'r');
    int res = fread(buf, sizeof(char), 25, fd);
    if(buf[1] + buf[14] == 'X') {
        if(buf[5] + buf[15] + buf[14] > buf[6] + buf[19]) {
            if(buf[19] + buf[23] == 'V') {
                printf("vuln1\n");
            } else {
                if(buf[24] - buf[5] * buf[11] == 'f') {
                    printf("vuln2\n");
                } else {
                    printf("br1\n");
                }
            }
        }
    } else {
        if(buf[10] < buf[13] && buf[22] > buf[20]) printf("vuln3\n");
        printf("br2\n");
    }
    return 0;
}