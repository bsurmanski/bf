#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv)
{
    FILE *f = fopen("test.bf", "r");
    fseek(f, 0, SEEK_END);
    size_t sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *program = malloc(sz);
    fread(program, 1, sz, f); 
    
    char *arr = calloc(1, 32 * 1024);
    char *ptr = arr;
    char *pc = program;
    while(pc >= program && pc <= program + sz)
    {
        char curChar = *pc;
        switch (curChar)
        {
            case '>': ptr++; break;
            case '<': ptr--; break;
            case '+': ++*ptr; break;
            case '-': --*ptr; break;
            case '.': putchar(*ptr); break;
            case ',': *ptr=getchar(); break;
            case '[': {if(!*ptr){while((*++pc) != ']');}}; break;
            case ']': {if(*ptr){while((*--pc) != '[');}}; break;
        }
        pc++;
    }
    free(arr);

    return 0;
}
