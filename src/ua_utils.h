/* UA utilities: lightweight header for tests and runtime */
#ifndef UA_UTILS_H
#define UA_UTILS_H

#include <stddef.h>

#define UA_OTHER "Other"

/* Normalize a UA string into a stable short token (up to len bytes) */
void normalize_ua_buf(const char *src, char *dst, int len);

/* Get normalized UA key with "Other" fallback for empty normalized UAs */
const char *get_normalized_ua_key(const char *useragent, char *buf, size_t len);

#endif /* UA_UTILS_H */
