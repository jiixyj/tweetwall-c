#include "alpha.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

static iconv_t alpha_converter;

int alpha_new(struct alpha_packet *packet, char type_code,
              char sign_address_high, char sign_address_low)
{
    packet->memstream = open_memstream(&packet->data, &packet->data_size);
    if (!packet->memstream) {
        perror("open_memstream");
        return 1;
    }
    fwrite("\x00\x00\x00\x00\x00" "\x01", 6, 1, packet->memstream);
    fputc(type_code, packet->memstream);
    fputc(sign_address_high, packet->memstream);
    fputc(sign_address_low, packet->memstream);

    return 0;
}

int alpha_send(struct alpha_packet *packet)
{
    if (packet->memstream) {
        fclose(packet->memstream);
        packet->memstream = NULL;
    }
    fwrite(packet->data, packet->data_size, 1, stdout);
    fflush(stdout);

    return 0;
}

int alpha_destroy(struct alpha_packet *packet)
{
    if (packet->memstream) {
        fclose(packet->memstream);
        packet->memstream = NULL;
    }
    free(packet->data);

    return 0;
}

int alpha_write_string(struct alpha_packet *packet, char file_label, char *string)
{
    char *converted = utf8_to_alpha(string);
    fwrite("\x02" "A", 2, 1, packet->memstream);
    fputc(file_label, packet->memstream);
    fwrite("\x1b" " a", 3, 1, packet->memstream);
    fwrite(converted, strlen(converted), 1, packet->memstream);
    free(converted);

    return 0;
}

int alpha_write_sound(struct alpha_packet *packet)
{
    fwrite("\x03" "\x02" "E(2" "\x00" "FE11", 10, 1, packet->memstream);

    return 0;
}

int alpha_write_special_one(struct alpha_packet *packet,
                            char label,
                            char *data, size_t data_size)
{
    fputc('E', packet->memstream);
    fputc(label, packet->memstream);
    if (!data || !data_size) {
        fwrite(data, data_size, 1, packet->memstream);
    }

    return 0;
}

int alpha_write_special_two(struct alpha_packet *packet,
                            char label_high, char label_low,
                            char *data, size_t data_size)
{
    fputc('E', packet->memstream);
    fputc(label_high, packet->memstream);
    fputc(label_low, packet->memstream);
    if (!data || !data_size) {
        fwrite(data, data_size, 1, packet->memstream);
    }

    return 0;
}

void alpha_write_closing(struct alpha_packet *packet)
{
    fwrite("\x04", 1, 1, packet->memstream);
}

char *utf8_to_alpha(char *utf8)
{
    size_t i;

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

int alpha_init(void)
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

void alpha_shutdown(void)
{
    iconv_close(alpha_converter);
}
