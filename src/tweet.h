#ifndef TWEET_H
#define TWEET_H

#include <string.h>

int tweet_get_packet(char **packet, size_t *packet_size);
void tweet_shutdown();

#endif /* end of include guard: TWEET_H */
