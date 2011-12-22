#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "entities.h"
#include "tweet.h"
#include "alpha.h"

int loop = 1;

int main(int argc, char *argv[])
{
    if (alpha_init()) {
        return EXIT_FAILURE;
    }

    /* The socket thread is responsible to catch SIGINT and to set loop to 0 */
    if (socket_start_listening_thread()) {
        return EXIT_FAILURE;
    }

    if (argc == 2 && !strcmp(argv[1], "--reset")) {
        struct alpha_packet packet;
        if (alpha_new(&packet, 'Z', '0', '0') == 0) {
            alpha_write_special_one(&packet, '$', NULL);
            alpha_write_closing(&packet);
            alpha_send(&packet);
            alpha_destroy(&packet);
        }
        exit(EXIT_SUCCESS);
    }

    {
        struct alpha_packet packet;
        if (alpha_new(&packet, 'Z', '0', '0') == 0) {
            alpha_write_special_one(&packet, '$', "AAU0800FFFF"
                                                  "BAU0100FFFF"
                                                  "CAU0800FFFF");
            alpha_write_closing(&packet);
            alpha_send(&packet);
            alpha_destroy(&packet);
        }
    }

    {
        struct alpha_packet packet;
        if (alpha_new(&packet, 'Z', '0', '0') == 0) {
            alpha_write_string(&packet, 'B', " c", "TWEET AN @FGRAUM ");
            alpha_write_closing(&packet);
            alpha_send(&packet);
            alpha_destroy(&packet);
        }
    }

    while (loop) {
        struct alpha_packet packet;

        if (alpha_new(&packet, 'Z', '0', '0') == 0) {
            char *tweet_string = NULL;
            switch (tweet_get_string(&tweet_string)) {
            /* new tweets have arrived */
            case 1:
                alpha_write_string(&packet, 'A', " a", tweet_string);
                alpha_write_sound(&packet);
                alpha_write_closing(&packet);
                alpha_send(&packet);  /* after this, fall through to case 0 */
            /* tweets are the same as last time; still need to free
             * tweet_string */
            case 0:
                free(tweet_string);
                break;
            /* some error in tweet_get_string */
            default:
                break;
            }
            alpha_destroy(&packet);
        }
        sleep(30);
    }

    socket_join_listening_thread();
    alpha_shutdown();
    tweet_shutdown();
    xmlCleanupParser();

    return 0;
}
