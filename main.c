#include <stdio.h>
#include <string.h>

#include <jansson.h>
#include "entities.h"

int main(int argc, char *argv[])
{
    const char *json = "{\"test\":\"&nbsp;\\u2619\\u2661\\u2665\\u2763\\u2764\\u2765\\u2766\\u2767&amp;&lt;&quot;Jürgen\"}";
    json_t *root;
    json_error_t error;
    json_t *commits;
    const char *message_text;
    char *result;

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
