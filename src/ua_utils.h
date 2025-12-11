/* UA utilities: lightweight header for tests and runtime */
#ifndef UA_UTILS_H
#define UA_UTILS_H

/* Normalize a UA string into a stable short token (up to len bytes) */
void normalize_ua_buf(const char *src, char *dst, int len);

#endif /* UA_UTILS_H */
