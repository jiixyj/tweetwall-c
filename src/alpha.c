#include "alpha.h"

#include <stdio.h>
#include <stdlib.h>

int alpha_string_packet(char *string, size_t length, char **packet, size_t *packet_size)
{
    size_t i;

    char *leading = "\x00\x00\x00\x00\x00" "\x01" "Z" "00" "\x02" "AA" "\x1b" " a";
    char *sound = "\x03" "\x02" "E(2" "\x00" "FE11";

    FILE *memstream = open_memstream(packet, packet_size);
    if (!memstream) {
        perror("open_memstream");
        return 1;
    }
    fwrite(leading, 15, 1, memstream);
    for (i = 0; i < length; ++i) {
        if (isascii(string[i])) {
            if (!isprint(string[i])) {
                continue;
            } else {
                fwrite(&string[i], 1, 1, memstream);
            }
        } else {
            char without_offset = string[i] - 96;

            fwrite("\x08", 1, 1, memstream);
            fwrite(&without_offset, 1, 1, memstream);
        }
    }

    // fwrite(sound, 10, 1, memstream);
    fwrite("\x04", 1, 1, memstream);
    fclose(memstream);

    return 0;
}
