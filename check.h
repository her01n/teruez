#ifndef check_h_included
#define check_h_included

/**
 * Abort the program if the call returns negative value
 */
#define checked(call) \
    if ((call) < 0) { \
        fprintf(stderr, "Error on %s:%d\n", __FILE__, __LINE__); \
        fprintf(stderr, "%s\n", "call"); \
        perror(NULL); \
    }

#endif
        
