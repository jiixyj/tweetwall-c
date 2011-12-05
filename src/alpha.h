#ifndef ALPHA_H
#define ALPHA_H

#include <string.h>
#include <stdio.h>
#include <iconv.h>

struct alpha_packet {
    char *data;
    size_t data_size;
    FILE *memstream;
};

int alpha_init(void);
void alpha_shutdown(void);

int alpha_new(struct alpha_packet *packet);
int alpha_send(struct alpha_packet *packet);
int alpha_destroy(struct alpha_packet *packet);

int alpha_write_string(struct alpha_packet *packet, char *string);
int alpha_write_sound(struct alpha_packet *packet);

void alpha_write_leading(struct alpha_packet *packet);
void alpha_write_closing(struct alpha_packet *packet);

char *utf8_to_alpha(char *utf8);

#endif /* end of include guard: ALPHA_H */
