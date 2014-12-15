#ifndef check_h_included
#define check_h_included

/**
 * Abort if rv equals -1
 * Returns rv.
 */
int check_return(int rv, const char* call, const char* file, int line);

/**
 * Abort the program if the call returns negative value
 */
#define checked(call) check_return((call), #call, __FILE__, __LINE__)

#endif
        
