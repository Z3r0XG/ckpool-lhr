#include "config.h"

#include <ctype.h>
#include <string.h>
#include <stddef.h>
#include "ua_utils.h"

/*
 * UA normalisation policy (applied in order after version-separator strip):
 *
 *  Rule 2 – Strip trailing chip-model suffix " BM<digits>" or "-BM<digits>".
 *           e.g. "LuckyMiner BM1366" → "LuckyMiner"
 *  Rule 3 – Collapse the cpuminer family to "cpuminer".
 *           cpuminer-multi, cpuminer-opt-v2xxx, cpuminer_gc3355, … all map to
 *           "cpuminer".  The separator must be '-' or '_'.
 *  Rule 4 – Strip trailing "-<digits>[.<digits>]*" version suffix.
 *           e.g. "xminer-1.2.7" → "xminer"
 *           Safe: "stratum-ping", "esp32s3-toy", "NerdOCTAXE-γ" are
 *           unaffected because the char after '-' is not a digit.
 */

static void strip_trailing_ws(char *s, int *lenp)
{
    while (*lenp > 0 && isspace((unsigned char)s[*lenp - 1]))
        s[--(*lenp)] = '\0';
}

/* Rule 2 */
static void strip_bm_suffix(char *s, int *lenp)
{
    int n = *lenp;
    int j = n;
    /* walk back over trailing digits */
    while (j > 0 && isdigit((unsigned char)s[j - 1]))
        j--;
    /* expect "BM" immediately before the digit run (j < n ensures at least one digit was consumed) */
    if (j < n && j >= 2 && s[j - 2] == 'B' && s[j - 1] == 'M') {
        int sep = j - 2; /* index of 'B' */
        if (sep > 0 && (s[sep - 1] == ' ' || s[sep - 1] == '-')) {
            *lenp = sep - 1;
            s[*lenp] = '\0';
        }
    }
}

/* Rule 4 */
static void strip_dash_version(char *s, int *lenp)
{
    int n = *lenp;
    int j = n;
    /* suffix must end with a digit (rejects trailing-dot cases like "foo-1.") */
    if (n == 0 || !isdigit((unsigned char)s[n - 1]))
        return;
    /* walk back over digits and dots */
    while (j > 0 && (isdigit((unsigned char)s[j - 1]) || s[j - 1] == '.'))
        j--;
    /* the suffix must be preceded by '-' and must begin with a digit */
    if (j < n && j > 0 && s[j - 1] == '-' && isdigit((unsigned char)s[j])) {
        *lenp = j - 1;
        s[*lenp] = '\0';
    }
}

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

    /* Copy until version separators '/' or '(' */
    while (*src && i < len - 1) {
        if (*src == '/' || *src == '(')
            break;
        dst[i++] = (unsigned char)*src;
        src++;
    }
    dst[i] = '\0';

    strip_trailing_ws(dst, &i);

    /* Rule 2: chip-model suffix */
    strip_bm_suffix(dst, &i);
    strip_trailing_ws(dst, &i);

    /* Rule 3: cpuminer family */
    if (i > 8 && strncasecmp(dst, "cpuminer", 8) == 0 &&
        (dst[8] == '-' || dst[8] == '_')) {
        dst[8] = '\0';
        i = 8;
    }

    /* Rule 4: dash-version suffix */
    strip_dash_version(dst, &i);
}

/* Get normalized UA key with "Other" fallback for empty normalized UAs */
const char *get_normalized_ua_key(const char *useragent, char *buf, size_t len)
{
    normalize_ua_buf(useragent, buf, len);
    return buf[0] != '\0' ? buf : UA_OTHER;
}
