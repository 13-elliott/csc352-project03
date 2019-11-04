#include <stdio.h>
#include "mymalloc.h"

int main() {
    int leng = 20;
    int *test = my_firstfit_malloc(sizeof(int)*leng);
    
    my_free(test);
}
