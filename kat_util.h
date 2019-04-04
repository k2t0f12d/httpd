/**
 * kat_util.h
 */

#ifndef __KAT_UTIL_H
#define __KAT_UTIL_H

internal void print_raw(char *message)
{
        char *current = message;
        while(*current)
        {
                switch(*current){
                        case '\r': printf("\\r"); break;
                        case '\n': printf("\\n"); break;
                        case '\t': printf("\\t"); break;
                        default: putchar(*current); break;
                }
                current++;
        }
}

#endif /* __KAT_UTIL_H */
