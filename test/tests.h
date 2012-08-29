#ifndef librope_test_h
#define librope_test_h

#include <stdint.h>

#define MIN(x,y) ((x) > (y) ? (y) : (x))

#ifdef __cplusplus
extern "C" {
#endif

void benchmark();

// len is approximate. Might use fewer bytes than that.
void random_unicode_string(uint8_t *buffer, size_t len);

// len includes \0.
void random_ascii_string(uint8_t *buffer, size_t len);  

#ifdef __cplusplus
}
#endif
    
#endif
