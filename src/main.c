#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <iconv.h>
#include <signal.h>

#include <jansson.h>
#include <curl/curl.h>

#include "entities.h"

#define NUMBER_OF_TWEETS 5
#define NUMBER_OF_TWEETS_STR "5"

static int loop = 1;

void exit_handler(int signal)
{
    fprintf(stderr, "Caught signal %d\n", signal);
    loop = 0;
}

struct tweet {
    char *from_user;
    char *id_str;
    char *text;
};

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

static void parse_tweets(char *json, struct tweet *tweets)
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
        json_t *result, *from_user, *text, *id_str;
        const char *from_user_cstr, *text_cstr, *id_str_cstr;

        if (!(result = json_array_get(results, i))) {
            fprintf(stderr, "Could not get results array from response\n");
            continue;
        }

        if (!(from_user = json_object_get(result, "from_user")) ||
               !(id_str = json_object_get(result, "id_str")) ||
                 !(text = json_object_get(result, "text"))) {
            fprintf(stderr, "Could not get specified field(s) from result\n");
            continue;
        }

        if (!(from_user_cstr = json_string_value(from_user)) ||
               !(id_str_cstr = json_string_value(id_str)) ||
                 !(text_cstr = json_string_value(text))) {
            fprintf(stderr, "Expected different type of result field\n");
            continue;
        }

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

static char *utf8_to_alpha(iconv_t alpha_converter, char *utf8)
{
    size_t inbytes = strlen(utf8);
    size_t outbytes = inbytes;
    char *alpha = calloc(outbytes + 1, 1);
    char *outp = alpha;
    size_t ret;

    ret = iconv(alpha_converter, &utf8, &inbytes, &outp, &outbytes);
    if (ret == (size_t) -1) {
        perror("iconv");
        alpha[0] = '\0';
    } else if (inbytes != 0) {
        fprintf(stderr, "iconv: string could not be fully converted\n");
    }
    return alpha;
}

static int compare_tweets(struct tweet *old, struct tweet *new)
{
    int i;
    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        if ((old[i].id_str && !new[i].id_str) ||
            (!old[i].id_str && new[i].id_str)) {
            return 1;
        }
        if (old[i].id_str && new[i].id_str) {
            if (strcmp(old[i].id_str, new[i].id_str)) {
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    iconv_t alpha_converter;
    static struct tweet last_tweets[NUMBER_OF_TWEETS];
    int i;

    signal(SIGINT, exit_handler);

    alpha_converter = iconv_open("ALPHA//TRANSLIT//IGNORE", "UTF-8");
    if (alpha_converter == (iconv_t) -1) {
        perror("iconv_open");
        fprintf(stderr, "Please set GCONV_PATH to directory where gconv-modules"
                        " for the ALPHA charset lies. This is src/iconv-alpha"
                        " for the CMake build.\n\n");
    }

    while (loop) {
        struct tweet tweets[NUMBER_OF_TWEETS] = { 0 };
        char *json;
        int new_tweets_different = 0;

        json = curl_fgraum_twitter();
        if (!json) {
            goto next;
        }
        parse_tweets(json, tweets);
        free(json);

        new_tweets_different = compare_tweets(last_tweets, tweets);
        if (!new_tweets_different) {
            fprintf(stderr, "New tweets are not different\n");
        }

        for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
            if (tweets[i].text) {
                if (alpha_converter != (iconv_t) -1) {
                    char *alpha = utf8_to_alpha(alpha_converter, tweets[i].text);
                    char *user_alpha = utf8_to_alpha(alpha_converter, tweets[i].from_user);

                    printf("utf-8: %s: %s\n", tweets[i].from_user, tweets[i].text + 8);
                    printf("alpha: %s: %s\n", user_alpha, alpha + 8);

                    free(alpha);
                    free(user_alpha);
                } else {
                    printf("%s: %s\n", tweets[i].from_user, tweets[i].text);
                }
            }

            free(last_tweets[i].from_user);
            free(last_tweets[i].id_str);
            free(last_tweets[i].text);

            last_tweets[i].from_user = tweets[i].from_user;
            last_tweets[i].id_str = tweets[i].id_str;
            last_tweets[i].text = tweets[i].text;
        }
        printf("\n");

      next:
        sleep(10);
    }

    for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
        free(last_tweets[i].from_user);
        free(last_tweets[i].id_str);
        free(last_tweets[i].text);
    }

    iconv_close(alpha_converter);

    return 0;
}
