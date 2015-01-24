#ifndef _PASPRINTF_H
#define _PASPRINTF_H

int pasprintf(char **strp, const char *fmt, ...);
int pvasprintf(char **strp, const char *fmt, va_list args);

#endif
