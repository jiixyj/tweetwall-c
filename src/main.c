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
    #pragma unused(argc, argv)

    signal(SIGINT, exit_handler);

    if (alpha_init()) {
        return EXIT_FAILURE;
    }

    while (loop) {
        struct alpha_packet packet;

        if (alpha_new(&packet, 'Z', '0', '0') == 0) {
            char *tweet_string = NULL;
            if (tweet_get_string(&tweet_string) == 0) {
                if (tweet_new_tweets_different()) {
                    alpha_write_string(&packet, tweet_string);
                    alpha_write_sound(&packet);
                    alpha_write_closing(&packet);
                    alpha_send(&packet);
                }
                free(tweet_string);
            }
            alpha_destroy(&packet);
        }

        sleep(30);
    }

    alpha_shutdown();
    tweet_shutdown();

    return 0;
}
