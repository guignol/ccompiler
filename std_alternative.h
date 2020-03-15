//#ifndef CCOMPILER_STD_ALTERNATIVE_H
//#define CCOMPILER_STD_ALTERNATIVE_H

/////////////////////////////////////////////////
//#include <string.h>

#define NULL ((void *)0)

char *strerror(int __errnum);

int memcmp(const void *__s1, const void *__s2, /** size_t */ int __n);

int strncmp(const char *__s1, const char *__s2, /** size_t */ int __n);

char *strncpy(char *__dest, const char *__src, /** size_t */ int __n);

char *strndup(const char *__string, /** size_t */ int __n);

char *strchr(const char *__s, int __c);

/** size_t */ int strlen(const char *__s);

/////////////////////////////////////////////////
//#include <stdio.h>

//extern /** struct _IO_FILE */ void *stdin;		/* Standard input stream.  */
extern /** struct _IO_FILE */ void *stdout;        /* Standard output stream.  */
extern /** struct _IO_FILE */ void *stderr;        /* Standard error output stream.  */

// -Wbuiltin-declaration-mismatchはCMAKEでオフにしている

int printf(); // int printf (const char *__format, ...)

int fprintf(); // int fprintf(FILE *__stream, const char *__format, ...);

int sprintf(); // int sprintf (char *__s, const char *__format, ...)

/** FILE */ void *fopen(const char *__filename, const char *__modes);

#define SEEK_SET    0    /* Seek from beginning of file.  */
//#define SEEK_CUR	1	/* Seek from current position.  */
#define SEEK_END    2    /* Seek from end of file.  */

int fseek(/** FILE */ void *__stream, /** long int */ int __off, int __whence);

/* TODO */ long int ftell(/** FILE */ void *__stream);

/** size_t */ int fread(void *__ptr, /** size_t */ int __size, /** size_t */ int __n, /** FILE */ void *__stream);

int fclose(/** FILE */ void *__stream);

/////////////////////////////////////////////////
//#include <stdlib.h>

void *malloc(/** size_t */ int __size);

void *calloc(/** size_t */ int __nmemb, /** size_t */ int __size);

void *realloc(void *__ptr, /** size_t */ int __size);

/** long int */ int strtol(const char *__nptr, char **__endptr, int __base);

void exit(int __status);

/////////////////////////////////////////////////
//#include <stdbool.h>

#define bool _Bool
#define true    1
#define false    0

/////////////////////////////////////////////////
//#include <errno.h>
int *__errno_location(void);

/////////////////////////////////////////////////

//#endif //CCOMPILER_STD_ALTERNATIVE_H
