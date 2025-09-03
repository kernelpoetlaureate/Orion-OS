#include <stdio.h>
#include "libc.h"

int main(void) {
    char buf[16];
    memset(buf, 'A', 15);
    buf[15] = '\0';
    printf("memset -> %s\n", buf);

    char src[] = "Hello";
    char dst[6];
    memcpy(dst, src, 6);
    printf("memcpy -> %s\n", dst);

    printf("strcmp eq -> %d\n", strcmp("abc", "abc"));
    printf("strcmp lt -> %d\n", strcmp("abc", "abd"));

    return 0;
}
