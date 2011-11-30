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
    json_t *root, *results;
    json_error_t error;
    char *json;
    CURL *curl;
    unsigned int i;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://search.twitter.com/search.json?q=to:fgraum&rpp=3");
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &json);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, callback_func);
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    root = json_loads(json, 0, &error);
    free(json);
    results = json_object_get(root, "results");
    for (i = 0; i < json_array_size(results); ++i) {
        json_t *result, *from_user, *text;
        const char *from_user_cstr, *text_cstr;
        char *from_user_esc, *text_esc;

        result = json_array_get(results, i);
        from_user = json_object_get(result, "from_user");
        text = json_object_get(result, "text");

        from_user_cstr = json_string_value(from_user);
        text_cstr = json_string_value(text);

        from_user_esc = strdup(from_user_cstr);
        text_esc = strdup(text_cstr);

        decode_html_entities_utf8(from_user_esc, NULL);
        decode_html_entities_utf8(text_esc, NULL);

        printf("%s: %s\n", from_user_esc, text_esc);

        free(from_user_esc);
        free(text_esc);
    }

    json_decref(root);
    return 0;
}
