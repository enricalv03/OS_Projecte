#include "kstring.h"

/* =============================================================================
 * Kernel String Library -- Implementation
 * =============================================================================
 * These are the only string/memory routines available in the kernel.
 * GCC may emit implicit calls to memset/memcpy for struct assignments
 * and initialization, so these MUST be present and use standard names.
 * =========================================================================== */

/* ---- Memory operations --------------------------------------------------- */

void* memset(void* dest, int val, unsigned int count) {
    unsigned char* d = (unsigned char*)dest;
    unsigned char v = (unsigned char)val;

    /* Fast path: fill 4 bytes at a time when aligned */
    if (count >= 4 && ((unsigned int)d & 3) == 0) {
        unsigned int v32 = v | (v << 8) | (v << 16) | (v << 24);
        unsigned int* d32 = (unsigned int*)d;
        while (count >= 4) {
            *d32++ = v32;
            count -= 4;
        }
        d = (unsigned char*)d32;
    }

    while (count--) {
        *d++ = v;
    }
    return dest;
}

void* memcpy(void* dest, const void* src, unsigned int count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    /* Fast path: copy 4 bytes at a time when both aligned */
    if (count >= 4 && (((unsigned int)d | (unsigned int)s) & 3) == 0) {
        unsigned int* d32 = (unsigned int*)d;
        const unsigned int* s32 = (const unsigned int*)s;
        while (count >= 4) {
            *d32++ = *s32++;
            count -= 4;
        }
        d = (unsigned char*)d32;
        s = (const unsigned char*)s32;
    }

    while (count--) {
        *d++ = *s++;
    }
    return dest;
}

void* memmove(void* dest, const void* src, unsigned int count) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;

    if (d < s) {
        /* Forward copy (same as memcpy) */
        while (count--) {
            *d++ = *s++;
        }
    } else if (d > s) {
        /* Backward copy to handle overlap */
        d += count;
        s += count;
        while (count--) {
            *--d = *--s;
        }
    }
    return dest;
}

int memcmp(const void* a, const void* b, unsigned int count) {
    const unsigned char* p1 = (const unsigned char*)a;
    const unsigned char* p2 = (const unsigned char*)b;

    while (count--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

/* ---- String operations --------------------------------------------------- */

unsigned int strlen(const char* str) {
    unsigned int len = 0;
    while (str[len]) {
        len++;
    }
    return len;
}

char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++) != 0)
        ;
    return dest;
}

char* strncpy(char* dest, const char* src, unsigned int n) {
    char* d = dest;
    while (n && (*d++ = *src++) != 0) {
        n--;
    }
    /* Pad remaining bytes with null if src was shorter */
    while (n--) {
        *d++ = 0;
    }
    return dest;
}

int strcmp(const char* a, const char* b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

int strncmp(const char* a, const char* b, unsigned int n) {
    if (n == 0) return 0;
    while (--n && *a && *a == *b) {
        a++;
        b++;
    }
    return *(unsigned char*)a - *(unsigned char*)b;
}

char* strchr(const char* str, int c) {
    while (*str) {
        if (*str == (char)c) {
            return (char*)str;
        }
        str++;
    }
    return (c == 0) ? (char*)str : 0;
}

char* strcat(char* dest, const char* src) {
    char* d = dest;
    while (*d) d++;
    while ((*d++ = *src++) != 0)
        ;
    return dest;
}
