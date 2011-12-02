#include "alpha.h"

#include <stdio.h>
#include <stdlib.h>

static iconv_t alpha_converter;

void alpha_write_leading(FILE *memstream)
{
    static const char *leading = "\x00\x00\x00\x00\x00" "\x01" "Z" "00" "\x02" "AA" "\x1b" " a";
    fwrite(leading, 15, 1, memstream);
}

void alpha_write_closing(FILE *memstream)
{
    fwrite("\x04", 1, 1, memstream);
}

char *utf8_to_alpha(char *utf8)
{
    int i;

    size_t inbytes = strlen(utf8);
    size_t outbytes = inbytes;
    char *alpha = calloc(outbytes + 1, 1);
    size_t alpha_length;
    char *outp = alpha;
    size_t ret;

    char *fully_converted;
    size_t fully_converted_size;
    FILE *memstream = open_memstream(&fully_converted, &fully_converted_size);
    if (!memstream) {
        perror("open_memstream");
        alpha[0] = '\0';
        return alpha;
    }

    ret = iconv(alpha_converter, &utf8, &inbytes, &outp, &outbytes);
    if (ret == (size_t) -1) {
        perror("iconv");
        alpha[0] = '\0';
    } else if (inbytes != 0) {
        fprintf(stderr, "iconv: string could not be fully converted\n");
    }

    alpha_length = strlen(alpha);

    for (i = 0; i < alpha_length; ++i) {
        if (isascii(alpha[i])) {
            if (!isprint(alpha[i])) {
                continue;
            } else {
                fwrite(&alpha[i], 1, 1, memstream);
            }
        } else {
            char without_offset = alpha[i] - 96;

            fwrite("\x08", 1, 1, memstream);
            fwrite(&without_offset, 1, 1, memstream);
        }
    }
    free(alpha);
    fclose(memstream);

    return fully_converted;
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
