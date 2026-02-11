#ifndef KSTRING_H
#define KSTRING_H

/* =============================================================================
 * Kernel String Library
 * =============================================================================
 * Freestanding implementations of essential string/memory functions.
 * These replace <string.h> since we compile with -nostdinc -nostdlib.
 * All functions are optimized for the kernel environment (no SSE, no FPU).
 * =========================================================================== */

/* Fill memory with a constant byte */
void* memset(void* dest, int val, unsigned int count);

/* Copy memory block (non-overlapping) */
void* memcpy(void* dest, const void* src, unsigned int count);

/* Copy memory block (handles overlapping regions) */
void* memmove(void* dest, const void* src, unsigned int count);

/* Compare two memory blocks */
int memcmp(const void* a, const void* b, unsigned int count);

/* Get string length (not counting null terminator) */
unsigned int strlen(const char* str);

/* Copy string (including null terminator) */
char* strcpy(char* dest, const char* src);

/* Copy at most n characters from src to dest */
char* strncpy(char* dest, const char* src, unsigned int n);

/* Compare two strings */
int strcmp(const char* a, const char* b);

/* Compare at most n characters of two strings */
int strncmp(const char* a, const char* b, unsigned int n);

/* Find first occurrence of character in string */
char* strchr(const char* str, int c);

/* Concatenate strings */
char* strcat(char* dest, const char* src);

#endif
