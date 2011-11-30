#include <stdio.h>
#include <string.h>

#include <jansson.h>
#include <curl/curl.h>

#include "entities.h"

static size_t callback_func(void *ptr, size_t size, size_t count, void *stream)
{
    char **json = (char **) stream;

    *json = malloc(size * count + 1);
    memcpy(*json, ptr, size * count);
    (*json)[size * count] = '\0';
    return size * count;
}

int main(int argc, char *argv[])
{
    char *json = NULL;
    json_t *root;
    json_error_t error;
    json_t *commits;
    const char *message_text;
    char *result;
    CURL *curl;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://search.twitter.com/search.json?q=to:fgraum&rpp=3");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_func);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        printf("%s\n", json);
    }

    root = json_loads(json, 0, &error);
    commits = json_object_get(root, "test");
    message_text = json_string_value(commits);
    result = strdup(message_text);
    decode_html_entities_utf8(result, NULL);
    printf("%s\n", result);
    free(result);
    json_decref(root);
    return 0;
}
