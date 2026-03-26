


// 1. String literal - stored in flash (.rodata) : read-only
const char *ptr = "hello";

// 2. String literal - stored in .rodata but at startup it gets copied to the stack
char buf1[20] = "hello";

// 3. String literal - same as above. Compiler sizes the array of fit buf[6] including '\0'
char buffer[] = "hello";       

// 4. Static allocation - stored in .data section. Persistent for the lifetime of the program.
static char buf[20] = "hello";      // Preferred over stack strings for buffers that are reused across calls






