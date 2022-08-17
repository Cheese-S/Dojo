#ifndef dojo_error_h
#define dojo_error_h

#include "scanner.h"

void errorAtToken(Token *token, const char *message);

void runtimeError(int line, const char *format, ...);
#endif
