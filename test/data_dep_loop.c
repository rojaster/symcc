#include <stdlib.h>
#include <stdio.h>

int main(int argc, char** argv) {
    char *file = argv[1];
    char str[2000];
    int fsize;
    int check=0, i;
    FILE *fp = fopen(file, 'r');
    fseek(fp, 0, SEEK_END);
    fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if(fsize < 24 || fsize > 1999) return -1;
    fread(str, sizeof(char), fsize, fp);
    for(i = 0; i < (fsize-6); ++i) {
        if(str[i] + str[i+1] < str[i+3] + str[i+4] + str[i+5]) check++;
    }
    if(check < 1) {
        printf("many large chars. Exiting!\n");
        return -1;
    }
    if(str[7] + str[5] == 'R'){
        printf("branch 1\n");
        if(str[18] + str[19] == 'B') {
            printf("branch 2\n");
            if(str[2] + str[4] == 'X') printf("unrelated branch 1\n");
            if(str[4] + str[8] == 'X') printf("unrelated branch 2\n");
            if(str[15] + str[18]) {
                printf("branch 3\n");
                if(str[5] + str[9] == 'X') printf("unrelated branch 3\n");
                if(str[11] + str[24] == 'X') printf("unrelated branch 4\n");
                if(str[15] + str[14] == 'G') {
                    printf("vuln branch 4\n"); return -127;
                } else printf("branch 5\n");
            } else printf("branch 6\n");
        }
    }
    printf("branch 0\n");
    return 0;
}