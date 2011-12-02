#ifndef ALPHA_H
#define ALPHA_H

#include <string.h>
#include <iconv.h>

int alpha_string_packet(char *string, size_t length, char **packet, size_t *packet_size);
char *utf8_to_alpha(char *utf8);

int alpha_init();

#endif /* end of include guard: ALPHA_H */
