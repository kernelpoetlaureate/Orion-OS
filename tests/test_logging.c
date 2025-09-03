#include "../kernel/core/log.h"

int main(void) {
    kprintf("Test: %s %d %x\n", "hello", 42, 0x2a);
    LOG(LOG_LEVEL_INFO, "Info message: %s", "it works");
    return 0;
}
