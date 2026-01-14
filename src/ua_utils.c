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
    
    /* Strip trailing whitespace */
    while (i > 0 && isspace((unsigned char)dst[i - 1])) {
        i--;
        dst[i] = '\0';
    }
}

/* Get normalized UA key with "Other" fallback for empty normalized UAs */
const char *get_normalized_ua_key(const char *useragent, char *buf, size_t len)
{
    normalize_ua_buf(useragent, buf, len);
    return buf[0] != '\0' ? buf : UA_OTHER;
}
