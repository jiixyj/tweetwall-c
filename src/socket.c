#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/epoll.h>
#include <netinet/in.h>
#include <netdb.h>

#include "alpha.h"

extern int loop;

#define MYPORT "1337"
#define MAX_EVENTS 10

static void socket_sigint_handler(int signal)
{
    fprintf(stderr, "Caught signal %d in thread\n", signal);
    loop = 0;
}

static void *socket_listen(void *arg)
{
    int err, i, one = 1;
    struct sigaction sa;

    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    struct addrinfo hints = { 0 }, *res;

    struct epoll_event ev = { 0 }, events[MAX_EVENTS] = { { 0 } };
    int listen_sock, conn_sock, nfds, epollfd;


    sa.sa_handler = socket_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((err = getaddrinfo(NULL, MYPORT, &hints, &res))) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
        return (void *) EXIT_FAILURE;
    }

    listen_sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (bind(listen_sock, res->ai_addr, res->ai_addrlen)) {
        perror("bind");
        return (void *) EXIT_FAILURE;
    }

    freeaddrinfo(res);

    if (listen(listen_sock, MAX_EVENTS)) {
        perror("listen");
        return (void *) EXIT_FAILURE;
    }

    epollfd = epoll_create(MAX_EVENTS);
    if (epollfd < 0) {
        perror("epoll_create");
        return (void *) EXIT_FAILURE;
    }

    ev.events = EPOLLIN;
    ev.data.fd = listen_sock;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, listen_sock, &ev)) {
        perror("epoll_ctl");
        return (void *) EXIT_FAILURE;
    }

    addr_size = sizeof their_addr;

    while (loop) {
        int timeout = -1;

        nfds = epoll_wait(epollfd, events, MAX_EVENTS, timeout);
        if (nfds == -1) {
            perror("epoll_pwait");
            break;
        }

        for (i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listen_sock) {
                conn_sock = accept(listen_sock,
                                   (struct sockaddr *) &their_addr, &addr_size);
                if (conn_sock == -1) {
                    perror("accept");
                    exit(EXIT_FAILURE);
                }
                ev.events = EPOLLIN | EPOLLET;
                ev.data.fd = conn_sock;
                if (epoll_ctl(epollfd, EPOLL_CTL_ADD, conn_sock, &ev)) {
                    perror("epoll_ctl: conn_sock");
                    exit(EXIT_FAILURE);
                }
            } else {
                int new_fd = events[i].data.fd;
                unsigned char buf[125];
                ssize_t ret = recv(new_fd, buf, 124, 0);
                if (ret == -1 || ret == 0) {
                    fprintf(stderr, "continue %d\n", (int) ret);
                    if (ret == 0) {
                        close(new_fd);
                    }
                    continue;
                }

                buf[ret] = '\0';
                {
                    struct alpha_packet packet;
                    if (alpha_new(&packet, 'Z', '0', '0') == 0) {
                        alpha_write_string(&packet, '0', " c", buf);
                        alpha_write_closing(&packet);
                        alpha_send(&packet);
                        alpha_destroy(&packet);
                    }
                }
            }
        }
    }

    close(listen_sock);
    close(epollfd);

    return (void *) 0;
}

static pthread_t thread;

void socket_start_listening_thread(void)
{
    pthread_create(&thread, NULL, socket_listen, NULL);
}

void socket_join_listening_thread(void)
{
    pthread_kill(thread, SIGINT);
    pthread_join(thread, NULL);
}
