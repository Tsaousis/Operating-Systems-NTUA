#include <stdio.h>
#include <unistd.h>
#include "zing2.h"
#include "zing.h"
void zing(void){
        printf("Hello team %s!\n", getlogin());
}

