#pragma once

#define ARRAY_SIZE(x) (sizeof(x)/sizeof(x[0]))

#define NORETURN __attribute__((noreturn))

#define barrier() do { __asm__ __volatile__ (""); } while (0)

