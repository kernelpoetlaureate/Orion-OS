#ifndef ORION_LOG_H
#define ORION_LOG_H

#include <stdarg.h>
#include <stddef.h>

/* Log levels */
#define LOG_LEVEL_TRACE 0
#define LOG_LEVEL_DEBUG 1
#define LOG_LEVEL_INFO  2
#define LOG_LEVEL_WARN  3
#define LOG_LEVEL_ERROR 4
#define LOG_LEVEL_PANIC 5

#ifndef LOG_LEVEL_MIN
#define LOG_LEVEL_MIN LOG_LEVEL_DEBUG
#endif

/* Forward declarations provided by lib/printf.c and drivers/serial.c */
void kprintf(const char *fmt, ...);
void panic(const char *fmt, ...);

/* Generic logging macro - internal */
#define _LOG_INTERNAL(level, fmt, ...) \
    do { kprintf("[%d] " fmt "\n", level, ##__VA_ARGS__); } while (0)

/* Compile-time filtered level macros */
#if LOG_LEVEL_TRACE >= LOG_LEVEL_MIN
#define LOG_TRACE(fmt, ...) _LOG_INTERNAL(LOG_LEVEL_TRACE, fmt, ##__VA_ARGS__)
#else
#define LOG_TRACE(fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL_DEBUG >= LOG_LEVEL_MIN
#define LOG_DEBUG(fmt, ...) _LOG_INTERNAL(LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL_INFO >= LOG_LEVEL_MIN
#define LOG_INFO(fmt, ...) _LOG_INTERNAL(LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#else
#define LOG_INFO(fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL_WARN >= LOG_LEVEL_MIN
#define LOG_WARN(fmt, ...) _LOG_INTERNAL(LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#else
#define LOG_WARN(fmt, ...) do {} while (0)
#endif

#if LOG_LEVEL_ERROR >= LOG_LEVEL_MIN
#define LOG_ERROR(fmt, ...) _LOG_INTERNAL(LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#else
#define LOG_ERROR(fmt, ...) do {} while (0)
#endif

/* Panic macro always compiled in */
#define PANIC(fmt, ...) panic("PANIC: " fmt, ##__VA_ARGS__)

#endif /* ORION_LOG_H */
