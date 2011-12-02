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

        if (tweet_get_packet(&packet, &packet_size)) {
            goto next;
        }

        fwrite(packet, packet_size, 1, stdout);
        fflush(stdout);

        free(packet);

      next:
        sleep(10);
    }

    alpha_shutdown();
    tweet_shutdown();

    return 0;
}
