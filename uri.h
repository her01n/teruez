#ifndef uri_h_included
#define uri_h_included 1

/**
 * Undo % escape characters.
 * At most size bytes are written to dest
 * Returns number of bytes written to dest without the null terminator.
 * Or -1 on error. errno will be set.
 */
int unescapeURI(char* dest, char* source, int size);

#endif

