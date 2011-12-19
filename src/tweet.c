#include "tweet.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <json/json.h>
#include <curl/curl.h>

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

static void tweet_move_tweets(void)
{
    int i;
    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        tweet_move(&last_tweets[i], &tweets[i]);
    }
}

static void tweet_free_current_tweets(void)
{
    int i;
    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        tweet_free(&tweets[i]);
    }
}

static int tweet_new_tweets_different(void)
{
    int i;
    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        if ((last_tweets[i].id_str && !tweets[i].id_str) ||
            (!last_tweets[i].id_str && tweets[i].id_str)) {
            tweet_move_tweets();
            return 1;
        }
        if (last_tweets[i].id_str && tweets[i].id_str) {
            if (strcmp(last_tweets[i].id_str, tweets[i].id_str)) {
                tweet_move_tweets();
                return 1;
            }
        }
    }
    tweet_move_tweets();

    return 0;
}

static void prepare_json_for_libjsonc(char *json)
{
    int i;
    while (*json) {
        if (*json == '\\') {
            ++json;
            if (*json == 'u') {
                for (i = 0; i < 4; ++i) {
                    if (isxdigit(*++json)) {
                        *json = tolower(*json);
                    } else {
                        break;
                    }
                }
            }
        }
        ++json;
    }
}

static int parse_tweets(char *json)
{
    int err = 1;
    json_tokener* tok;
    json_object *root = NULL, *results;
    unsigned int i;

    prepare_json_for_libjsonc(json);

    tok = json_tokener_new();
    if (!tok) {
        fprintf(stderr, "Could not initialize tokenizer\n");
        goto cleanup;
    }
    root = json_tokener_parse_ex(tok, json, -1);
    if (tok->err != json_tokener_success) {
        fprintf(stderr, "json_tokener_parse_ex: error %s at offset %d\n",
                        json_tokener_errors[tok->err], tok->char_offset);
        goto cleanup;
    }

    if (!root) {
        fprintf(stderr, "Could not parse twitter response\n");
        goto cleanup;
    }

    if (!(results = json_object_object_get(root, "results"))) {
        fprintf(stderr, "Could not find 'results' in twitter response\n");
        goto cleanup;
    }

    if (json_object_get_type(results) != json_type_array) {
        fprintf(stderr, "'results' in twitter response is no array\n");
        goto cleanup;
    }

    for (i = 0; i < json_object_array_length(results); ++i) {
        json_object *result, *created_at, *from_user, *text, *id_str;
        const char *created_at_cstr, *from_user_cstr, *text_cstr, *id_str_cstr;

        if (!(result = json_object_array_get_idx(results, i))) {
            fprintf(stderr, "Could not get results array from response\n");
            continue;
        }

        if (!(created_at = json_object_object_get(result, "created_at")) ||
             !(from_user = json_object_object_get(result, "from_user")) ||
                !(id_str = json_object_object_get(result, "id_str")) ||
                  !(text = json_object_object_get(result, "text"))) {
            fprintf(stderr, "Could not get specified field(s) from result\n");
            continue;
        }

        if (!(created_at_cstr = json_object_get_string(created_at)) ||
             !(from_user_cstr = json_object_get_string(from_user)) ||
                !(id_str_cstr = json_object_get_string(id_str)) ||
                  !(text_cstr = json_object_get_string(text))) {
            fprintf(stderr, "Expected different type of result field\n");
            continue;
        }

        tweets[i].created_at = strdup(created_at_cstr);
        tweets[i].from_user = strdup(from_user_cstr);
        tweets[i].text = strdup(text_cstr);
        tweets[i].id_str = strdup(id_str_cstr);

        decode_html_entities_utf8(tweets[i].from_user);
        decode_html_entities_utf8(tweets[i].text);
    }

    err = 0;

  cleanup:
    json_tokener_free(tok);
    if (root) {
        json_object_put(root);
        root = NULL;
    }
    return err;
}

static int build_string_for_pager(char **string, size_t *string_size)
{
    int i;
    FILE *memstream = open_memstream(string, string_size);
    if (!memstream) {
        perror("open_memstream");
        return 1;
    }

    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        if (tweets[i].text) {
            char *created_at = strdup(tweets[i].created_at);
            char *token;

            fputs("               ", memstream);
            fputs(tweets[i].from_user, memstream);
            fputs(": ", memstream);
            fputs(tweets[i].text + 8, memstream);

            fputs(" --- ", memstream);
            if (strtok(created_at, " ")) {
                if ((token = strtok(NULL, " "))) {
                    fputs(token, memstream);
                    fputs(" ", memstream);
                    if ((token = strtok(NULL, " "))) {
                        fputs(token, memstream);
                        fputs(" ", memstream);
                        if (strtok(NULL, " ") && (token = strtok(NULL, " "))) {
                            fputs(token, memstream);
                            fputs(" ", memstream);
                        }
                    }
                }
            }

            free(created_at);
        }
    }

    fclose(memstream);

    return 0;
}

int tweet_get_string(char **string)
{
    char *json;
    size_t string_size;

    json = curl_fgraum_twitter();
    if (!json) {
        return -1;
    }
    if (parse_tweets(json)) {
        free(json);
        return -1;
    } else {
        free(json);
    }

    if (build_string_for_pager(string, &string_size)) {
        tweet_free_current_tweets();
        return -1;
    }

    if (tweet_new_tweets_different()) {
        return 1;
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
