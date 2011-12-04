#include "tweet.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <jansson.h>
#include <curl/curl.h>

#include "alpha.h"
#include "entities.h"

#define NUMBER_OF_TWEETS 3
#define NUMBER_OF_TWEETS_STR "3"

struct tweet {
    char *created_at;
    char *from_user;
    char *id_str;
    char *text;
};

static void tweet_free(struct tweet *tweet)
{
    free(tweet->created_at);
    free(tweet->from_user);
    free(tweet->id_str);
    free(tweet->text);
    tweet->created_at = NULL;
    tweet->from_user = NULL;
    tweet->id_str = NULL;
    tweet->text = NULL;
}

static void tweet_move(struct tweet *to, struct tweet *from)
{
    tweet_free(to);

    to->created_at = from->created_at;
    to->from_user = from->from_user;
    to->id_str = from->id_str;
    to->text = from->text;
    from->created_at = NULL;
    from->from_user = NULL;
    from->id_str = NULL;
    from->text = NULL;
}

static struct tweet last_tweets[NUMBER_OF_TWEETS];
static struct tweet tweets[NUMBER_OF_TWEETS];

static CURL *twitter_curl_init(FILE *memstream)
{
    CURLcode err;
    CURL *twitter_curl = curl_easy_init();
    char *url = "http://search.twitter.com/"
                "search.json?"
                "q=to:fgraum&"
                "rpp=" NUMBER_OF_TWEETS_STR;

    if (!twitter_curl) {
        fprintf(stderr, "Error initializing curl\n");
        return NULL;
    }
    if ((err = curl_easy_setopt(twitter_curl, CURLOPT_URL, url)) ||
        (err = curl_easy_setopt(twitter_curl, CURLOPT_WRITEDATA, memstream)) ||
        (err = curl_easy_setopt(twitter_curl, CURLOPT_WRITEFUNCTION, fwrite))) {
        fprintf(stderr, "%s\n", curl_easy_strerror(err));
        curl_easy_cleanup(twitter_curl);
        return NULL;
    }

    return twitter_curl;
}

static char *curl_fgraum_twitter()
{
    FILE *memstream = NULL;
    char *json = NULL;
    size_t json_size;
    CURL *twitter_curl = NULL;
    CURLcode errornum;

    if (errno = 0, !(memstream = open_memstream(&json, &json_size))) {
        perror("open_memstream");
        goto cleanup;
    }

    if (!(twitter_curl = twitter_curl_init(memstream))) {
        goto cleanup;
    }

    if ((errornum = curl_easy_perform(twitter_curl)) != CURLE_OK) {
        fprintf(stderr, "%s\n", curl_easy_strerror(errornum));
        goto cleanup;
    }

    curl_easy_cleanup(twitter_curl);
    fclose(memstream);
    return json;

  cleanup:
    if (twitter_curl)
        curl_easy_cleanup(twitter_curl);
    if (memstream)
        fclose(memstream);
    free(json);
    return NULL;
}

static int new_tweets_different()
{
    int i;
    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        if ((last_tweets[i].id_str && !tweets[i].id_str) ||
            (!last_tweets[i].id_str && tweets[i].id_str)) {
            return 1;
        }
        if (last_tweets[i].id_str && tweets[i].id_str) {
            if (strcmp(last_tweets[i].id_str, tweets[i].id_str)) {
                return 1;
            }
        }
    }
    return 0;
}

static void parse_tweets(char *json)
{
    json_t *root = NULL, *results;
    json_error_t error;
    unsigned int i;

    if (!(root = json_loads(json, 0, &error))) {
        fprintf(stderr, "Could not parse twitter response\n");
        goto cleanup;
    }

    if (!(results = json_object_get(root, "results"))) {
        fprintf(stderr, "Could not find 'results' in twitter response\n");
        goto cleanup;
    }

    if (!json_is_array(results)) {
        fprintf(stderr, "'results' in twitter response is no array\n");
        goto cleanup;
    }

    for (i = 0; i < json_array_size(results); ++i) {
        json_t *result, *created_at, *from_user, *text, *id_str;
        const char *created_at_cstr, *from_user_cstr, *text_cstr, *id_str_cstr;

        if (!(result = json_array_get(results, i))) {
            fprintf(stderr, "Could not get results array from response\n");
            continue;
        }

        if (!(created_at = json_object_get(result, "created_at")) ||
             !(from_user = json_object_get(result, "from_user")) ||
                !(id_str = json_object_get(result, "id_str")) ||
                  !(text = json_object_get(result, "text"))) {
            fprintf(stderr, "Could not get specified field(s) from result\n");
            continue;
        }

        if (!(created_at_cstr = json_string_value(created_at)) ||
             !(from_user_cstr = json_string_value(from_user)) ||
                !(id_str_cstr = json_string_value(id_str)) ||
                  !(text_cstr = json_string_value(text))) {
            fprintf(stderr, "Expected different type of result field\n");
            continue;
        }

        tweets[i].created_at = strdup(created_at_cstr);
        tweets[i].from_user = strdup(from_user_cstr);
        tweets[i].text = strdup(text_cstr);
        tweets[i].id_str = strdup(id_str_cstr);

        decode_html_entities_utf8(tweets[i].from_user, NULL);
        decode_html_entities_utf8(tweets[i].text, NULL);
    }

  cleanup:
    if (root) {
        json_decref(root);
        root = NULL;
    }
}

static int build_string_for_pager(FILE *memstream)
{
    int i;
    char *sound = "\x03" "\x02" "E(2" "\x00" "FE11";

    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        if (tweets[i].text) {
            char *created_at = strdup(tweets[i].created_at);
            char *token;
            char *alpha = utf8_to_alpha(tweets[i].text);
            char *user_alpha = utf8_to_alpha(tweets[i].from_user);

            fwrite(user_alpha, strlen(user_alpha), 1, memstream);
            fwrite(": ", 2, 1, memstream);
            fwrite(alpha + 8, strlen(alpha) - 8, 1, memstream);

            fwrite(" --- ", 5, 1, memstream);
            if (strtok(created_at, " ")) {
                if ((token = strtok(NULL, " "))) {
                    fwrite(token, strlen(token), 1, memstream);
                    fwrite(" ", 1, 1, memstream);
                    if ((token = strtok(NULL, " "))) {
                        fwrite(token, strlen(token), 1, memstream);
                        fwrite(" ", 1, 1, memstream);
                        if (strtok(NULL, " ") && (token = strtok(NULL, " "))) {
                            fwrite(token, strlen(token), 1, memstream);
                            fwrite(" ", 1, 1, memstream);
                        }
                    }
                }
            }

            if (i != NUMBER_OF_TWEETS - 1) {
                fwrite("          ", 10, 1, memstream);
            }

            free(created_at);
            free(alpha);
            free(user_alpha);
        }
    }

    if (new_tweets_different()) {
        fwrite(sound, 10, 1, memstream);
    }

    return 0;
}

int tweet_write(FILE *memstream)
{
    int i;
    char *json;

    json = curl_fgraum_twitter();
    if (!json) {
        return 1;
    }
    parse_tweets(json);
    free(json);

    build_string_for_pager(memstream);

    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        tweet_move(&last_tweets[i], &tweets[i]);
    }

    return 0;
}

void tweet_shutdown(void)
{
    int i;

    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        tweet_free(&last_tweets[i]);
    }
}
