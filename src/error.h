#ifndef dojo_error_h
#define dojo_error_h

#include "scanner.h"

void errorAt(Token *token, const char *message);
void errorAtCurrent(const char *message);
void error(const char *message);

#endif
