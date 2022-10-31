# Tasex 
Reducing symex footprint through filtering unnecessary dependencies

## Description


## Microbench

### Considered case: 

```cpp
#include <stdlib.h>
#include <stdio.h>

int main(int argc |  char** argv) {
    char *file = argv[1];
    char str[2000];
    int fsize;
    int check=0 |  i;
    FILE *fp = fopen(file |  "r");
    fseek(fp |  0 |  SEEK_END);
    fsize = ftell(fp);
    fseek(fp |  0 |  SEEK_SET);
    if(fsize < 24 || fsize > 1999) return -1;
    fread(str |  sizeof(char) |  fsize |  fp);
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
```
### Commands to run


```bash
cd /tmp
mkdir output
SYMCC_INPUT_FILE=/symcc_source/test/data_dep_loop.input ./test.sym /symcc_source/test/data_dep_loop.input 
SYMCC_INPUT_FILE=output/000001 ./test.sym output/000001
SYMCC_INPUT_FILE=output/000002 ./test.sym output/000002
for i in $(ls /tmp/output); do echo $i && ./test.elf /tmp/output/$i; done;
```

### Results

> 0 case - means that we re-run with new input produced on prev iteration.


#### **Tasex Applied**

* 1 - input size 29 bytes

| Case | Solving Time(s) | Syncing Time(s) | Skipped Constraints | Added Constraints | Symbolic Variables | Concrete Variables |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 0.204686 | 7.8884e-05 | 0 | 1 | 28 | 0 |
| 1 | 0.00637164 | 0.00158085 | 0 | 2 | 2 | 26 |
| 0 | 0.211929 | 6.6884e-05 | 0 | 1 | 28 | 0 |
| 1 | 0.00622718 | 0.00142599 | 0 | 2 | 2 | 26 |
| 2 | 0.00382074 | 0.00138111 | 1 | 2 | 2 | 26 |
| 0 | 0.207735 | 6.7363e-05 | 0 | 1 | 28 | 0 |
| 1 | 0.00630398 | 0.0018726 | 0 | 2 | 2 | 26 |
| 2 | 0.00350763 | 0.00145967 | 1 | 2 | 2 | 26 |
| 3 | 0.00356097 | 0.00208846 | 2 | 2 | 2 | 26 |
| 4 | 0.00382188 | 0.00121767 | 2 | 3 | 2 | 26 |
| 5 | 0.000641375 | 0.00152107 | 3 | 3 | 2 | 26 |
| 6 | 0.000761477 | 0.00166851 | 4 | 3 | 2 | 26 |
| 7 | 0.00366309 | 0.00118326 | 6 | 2 | 2 | 26 |
| 8 | 0.00369232 | 0.00113028 | 6 | 3 | 2 | 26 |

* 2 - input size 87 bytes

| Case | Solving Time(s) | Syncing Time(s) | Skipped Constraints | Added Constraints | Symbolic Variables | Concrete Variables |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 1.53814 | 0.000451071 | 0 | 1 | 86 | 0 |
| 1 | 0.00542425 | 0.00189231 | 0 | 2 | 2 | 84 |
| 0 | 1.51975 | 0.000456118 | 0 | 1 | 86 | 0 |
| 1 | 0.00550166 | 0.001952 | 0 | 2 | 2 | 84 | 
| 2 | 0.00480371 | 0.00168846 | 1 | 2 | 2 | 84 |
| 0 | 1.53148 | 0.000546146 | 0 | 1 | 86 | 0 |
| 1 | 0.00539983 | 0.00222923 | 0 | 2 | 2 | 84 |
| 2 | 0.00500311 | 0.00203918 | 1 | 2 | 2 | 84 |
| 3 | 0.00503107 | 0.00203759 | 2 | 2 | 2 | 84 |
| 4 | 0.00518459 | 0.00244954 | 2 | 3 | 2 | 84 |
| 5 | 0.000688421 | 0.00263542 | 3 | 3 | 2 | 84 |
| 6 | 0.00122265 | 0.0032857 | 4 | 3 | 2 | 84 |
| 7 | 0.00497389 | 0.00272653 | 6 | 2 | 2 | 84 |
| 8 | 0.00527623 | 0.00245546 | 6 | 3 | 2 | 84 |

* 3 - input size 118 bytes

| Case | Solving Time(s) | Syncing Time(s) | Skipped Constraints | Added Constraints | Symbolic Variables | Concrete Variables |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 9.90859 | 0.000803455 | 0 | 1 | 117 | 0 |
| 1 | 0.00604444 | 0.00208131 | 0 | 2 | 2 | 115 |
| 0 | 9.95625 | 0.000796323 | 0 | 1 | 117 | 0 |
| 1 | 0.00638975 | 0.00247146 | 0 | 2 | 2 | 115 |
| 2 | 0.00632454 | 0.00278371 | 1 | 2 | 2 | 115 | 
| 0 | 9.97024 | 0.000635271 | 0 | 1 | 117 | 0 |
| 1 | 0.00682076 | 0.00377579 | 0 | 2 | 2 | 115 |
| 2 | 0.00639711 | 0.00249095 | 1 | 2 | 2 | 115 |
| 3 | 0.00614146 | 0.00289338 | 2 | 2 | 2 | 115 |
| 4 | 0.00607706 | 0.00310493 | 2 | 3 | 2 | 115 |
| 5 | 0.000774065 | 0.00279517 | 3 | 3 | 2 | 115 |
| 6 | 0.000775241 | 0.00249981 | 4 | 3 | 2 | 115 |
| 7 | 0.00668184 | 0.00292849 | 6 | 2 | 2 | 115 |
| 8 | 0.00629879 | 0.0031358 | 6 | 3 | 2 | 115 |

#### **Original mode**

* 1 - input size 29 bytes

| Case | Solving Time(s) | Syncing Time(s) | Skipped Constraints | Added Constraints | Symbolic Variables | Concrete Variables |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 0.169764 | 9.7914e-05 | 0 | 1 | 28 | 0 |
| 1 | 0.135193 | 0.000912132 | 0 | 2 | 28 | 0 |
| 0 | 0.154122 | 0.000102083 | 0 | 1 | 28 | 0 |
| 1 | 0.115463 | 0.0005065 | 0 | 2 | 28 | 0 |
| 2 | 0.0633607 | 0.000472281 | 0 | 3 | 28 | 0 |
| 0 | 0.157857 | 9.9824e-05 | 0 | 1 | 28 | 0 |
| 1 | 0.107102 | 0.000475684 | 0 | 2 | 28 | 0 |
| 2 | 0.115351 | 0.00085396 | 0 | 3 | 28 | 0 |
| 3 | 0.116867 | 0.00112033 | 0 | 4 | 28 | 0 |
| 4 | 0.134797 | 0.00095575 | 0 | 5 | 28 | 0 |
| 5 | 0.126985 | 0.00110446 | 0 | 6 | 28 | 0 |
| 6 | 0.130875 | 0.00121945 | 0 | 7 | 28 | 0 |
| 7 | 0.0878564 | 0.00113115 | 0 | 8 | 28 | 0 |
| 8 | 0.16556 | 0.00161601 | 0 | 9 | 28 | 0 |

* 2 - input size 87 bytes

| Case | Solving Time(s) | Syncing Time(s) | Skipped Constraints | Added Constraints | Symbolic Variables | Concrete Variables |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 2.67816 | 0.00047161 | 0 | 1 | 86 | 0 |
| 1 | 0.611596 | 0.000672002 | 0 | 2 | 86 | 0 |
| 0 | 2.65857 | 0.000490342 | 0 | 1 | 86 | 0 |
| 1 | 0.545011 | 0.000539179 | 0 | 2 | 86 | 0 |
| 2 | 0.506275 | 0.000804234 | 0 | 3 | 86 | 0 |
| 0 | 2.68104 | 0.000594789 | 0 | 1 | 86 | 0 |
| 1 | 0.54011 | 0.000562683 | 0 | 2 | 86 | 0 |
| 2 | 0.564406 | 0.000892085 | 0 | 3 | 86 | 0 |
| 3 | 0.579714 | 0.000657294 | 0 | 4 | 86 | 0 |
| 4 | 0.568049 | 0.000971278 | 0 | 5 | 86 | 0 |
| 5 | 0.588808 | 0.000834218 | 0 | 6 | 86 | 0 |
| 6 | 0.521364 | 0.000820866 | 0 | 7 | 86 | 0 | 
| 7 | 0.573626 | 0.000908951 | 0 | 8 | 86 | 0 |
| 8 | 0.596315 | 0.000836725 | 0 | 9 | 86 | 0 |

* 3 - input size 118 bytes

| Case | Solving Time(s) | Syncing Time(s) | Skipped Constraints | Added Constraints | Symbolic Variables | Concrete Variables |
| --- | --- | --- | --- | --- | --- | --- |
| 0 | 7.27902 | 0.000835172 | 0 | 1 | 117 | 0 |
| 1 | 0.87828 | 0.000869411 | 0 | 2 | 117 | 0 |
| 0 | 7.23663 | 0.000826071 | 0 | 1 | 117 | 0 |
| 1 | 0.922265 | 0.00072193 | 0 | 2 | 117 | 0 |
| 2 | 0.86065 | 0.000715455 | 0 | 3 | 117 | 0 |
| 0 | 7.23504 | 0.00077129 | 0 | 1 | 117 | 0 |
| 1 | 0.921659 | 0.000647361 | 0 | 2 | 117 | 0 |
| 2 | 0.886137 | 0.000691459 | 0 | 3 | 117 | 0 |
| 3 | 0.912451 | 0.000676704 | 0 | 4 | 117 | 0 |
| 4 | 0.877003 | 0.000755587 | 0 | 5 | 117 | 0 |
| 5 | 0.967839 | 0.00103126 | 0 | 6 | 117 | 0 |
| 6 | 0.930051 | 0.000845529 | 0 | 7 | 117 | 0 |
| 7 | 0.964842 | 0.000872524 | 0 | 8 | 117 | 0 |
| 8 | 0.893414 | 0.00095384 | 0 | 9 | 117 | 0 |

> In original version there is no skipped constraints |  all dependant constraints are added to the solver. Also |  there is no concretized variables |  all variables are considered as symbolic.
>
> In original version |  syncing time might be faster because there is no checking what dependencies should be skipped because some of the variables could be considered as concrete.