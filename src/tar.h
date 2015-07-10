#ifndef _TT_TAR_H
#define _TT_TAR_H

#include<cstddef>
#include<cstdio>
#include<cstdint>

size_t output_dir(FILE *target, const char *name);
size_t output_file(FILE *target, FILE *input, const char *name, size_t size, uint32_t *crc);
size_t output_end(FILE *target);
void output_end_pad(FILE *target, size_t total_output_size);

#endif
