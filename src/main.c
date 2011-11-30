#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <iconv.h>

#include <jansson.h>
#include <curl/curl.h>

#include "entities.h"

#define NUMBER_OF_TWEETS 5
#define NUMBER_OF_TWEETS_STR "5"

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

int main(int argc, char *argv[])
{
    iconv_t alpha_converter;

    alpha_converter = iconv_open("ALPHA//TRANSLIT//IGNORE", "UTF-8");
    if (alpha_converter == (iconv_t) -1) {
        perror("iconv_open");
        fprintf(stderr, "Please set GCONV_PATH to directory where gconv-modules"
                        " for the ALPHA charset lies. This is src/iconv-alpha"
                        " for the CMake build.\n\n");
    }

    for (;;) {
        int i;
        struct tweet tweets[NUMBER_OF_TWEETS] = { 0 };
        char *json;

        json = curl_fgraum_twitter();
        if (!json) {
            goto next;
        }
        parse_tweets(json, tweets);
        free(json);

        for (i = 0; i < NUMBER_OF_TWEETS; ++i) {
            if (tweets[i].text) {
                if (alpha_converter != (iconv_t) -1) {
                    size_t inbytes = strlen(tweets[i].text);
                    size_t outbytes = inbytes;
                    char *alpha = calloc(outbytes + 1, 1);
                    char *inp = tweets[i].text;
                    char *outp = alpha;

                    iconv(alpha_converter, &inp, &inbytes, &outp, &outbytes);
                    printf("utf-8: %s\n", tweets[i].text);
                    printf("alpha: %s\n", alpha);

                    free(alpha);
                } else {
                    printf("%s\n", tweets[i].text);
                }
            }

            free(tweets[i].from_user);
            free(tweets[i].id_str);
            free(tweets[i].text);
        }
        printf("\n");

      next:
        sleep(10);
    }

    return 0;
}
