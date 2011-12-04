#ifndef ALPHA_H
#define ALPHA_H

#include <string.h>
#include <stdio.h>
#include <iconv.h>

int alpha_init(void);
void alpha_shutdown(void);

void alpha_write_leading(FILE *memstream);
void alpha_write_closing(FILE *memstream);

char *utf8_to_alpha(char *utf8);

#endif /* end of include guard: ALPHA_H */
