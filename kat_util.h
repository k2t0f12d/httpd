/**
 * kat_util.h
 */

#ifndef __KAT_UTIL_H
#define __KAT_UTIL_H

#include <stdio.h>
#include "kat.h"

internal void print_raw(char *message)
{
        char *current = message;
        while(*current)
        {
                switch(*current){
                        case '\r': fprintf(stderr,"\\r"); break;
                        case '\n': fprintf(stderr,"\\n"); break;
                        case '\t': fprintf(stderr,"\\t"); break;
                        default: fputc(*current, stderr); break;
                }
                current++;
        }
}

#endif /* __KAT_UTIL_H */
