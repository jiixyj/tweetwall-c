#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "entities.h"
#include "tweet.h"
#include "alpha.h"

static int loop = 1;

static void exit_handler(int signal)
{
    fprintf(stderr, "Caught signal %d\n", signal);
    loop = 0;
}

int main(int argc, char *argv[])
{
    signal(SIGINT, exit_handler);

    if (alpha_init()) {
        return EXIT_FAILURE;
    }

    while (loop) {
        char *packet;
        size_t packet_size;
        FILE *packet_memstream = open_memstream(&packet, &packet_size);
        if (!packet_memstream) {
            perror("open_memstream");
            goto next;
        }

        alpha_write_leading(packet_memstream);
        tweet_write(packet_memstream);
        alpha_write_closing(packet_memstream);

        fclose(packet_memstream);
        fwrite(packet, packet_size, 1, stdout);
        fflush(stdout);
        free(packet);

      next:
        sleep(30);
    }

    alpha_shutdown();
    tweet_shutdown();

    return 0;
}
