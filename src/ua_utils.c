#include <ctype.h>
#include <stddef.h>
#include "ua_utils.h"

void normalize_ua_buf(const char *src, char *dst, int len)
{
    int i = 0;
    if (!src || !dst || len <= 0) {
        if (dst && len > 0)
            dst[0] = '\0';
        return;
    }
    /* Skip leading whitespace */
    while (*src && isspace((unsigned char)*src)) src++;

    while (*src && i < len - 1) {
        if (*src == '/' || *src == '(')
            break;
        dst[i++] = (unsigned char)*src;
        src++;
    }
    dst[i] = '\0';
}
