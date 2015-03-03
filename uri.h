#ifndef uri_h_included
#define uri_h_included 1

/**
 * Undo % escape characters in place.
 * Returns 0 on success or -1 on error. errno will be set.
 */
int unescapeURI(char* uri);

#endif

