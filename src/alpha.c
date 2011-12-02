#include "alpha.h"

#include <stdio.h>
#include <stdlib.h>

static iconv_t alpha_converter;

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

    fwrite(sound, 10, 1, memstream);
    fwrite("\x04", 1, 1, memstream);
    fclose(memstream);

    return 0;
}

char *utf8_to_alpha(char *utf8)
{
    size_t inbytes = strlen(utf8);
    size_t outbytes = inbytes;
    char *alpha = calloc(outbytes + 1, 1);
    char *outp = alpha;
    size_t ret;

    ret = iconv(alpha_converter, &utf8, &inbytes, &outp, &outbytes);
    if (ret == (size_t) -1) {
        perror("iconv");
        alpha[0] = '\0';
    } else if (inbytes != 0) {
        fprintf(stderr, "iconv: string could not be fully converted\n");
    }
    return alpha;
}

int alpha_init()
{
    alpha_converter = iconv_open("ALPHA//TRANSLIT//IGNORE", "UTF-8");
    if (alpha_converter == (iconv_t) -1) {
        perror("iconv_open");
        fprintf(stderr, "Please set GCONV_PATH to directory where gconv-modules"
                        " for the ALPHA charset lies. This is src/iconv-alpha"
                        " for the CMake build.\n\n");
        return 1;
    }
    return 0;
}

void alpha_shutdown()
{
    iconv_close(alpha_converter);
}
