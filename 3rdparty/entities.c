#include "entities.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <libxml/HTMLparser.h>

struct charac_context {
    char *string;
    int i;
};

static void charac(void *cc_ptr, const xmlChar *chars, int length)
{
    struct charac_context *cc = (struct charac_context *) cc_ptr;
    memcpy(cc->string + cc->i, chars, (size_t) length);
    cc->i += length;
}

int decode_html_entities_utf8(char *string)
{
    static htmlSAXHandler saxHandler = { .characters = charac };
    struct charac_context cc = { string, 0 };
    htmlParserCtxtPtr ctxt;

    ctxt = htmlCreatePushParserCtxt(&saxHandler, &cc, NULL, 0, NULL,
                                    XML_CHAR_ENCODING_UTF8);
    if (!ctxt) {
        return 1;
    }
    if (htmlParseChunk(ctxt, string, (int) strlen(string), 1)) {
        return 1;
    }
    htmlFreeParserCtxt(ctxt);
    string[cc.i] = '\0';
    return 0;
}
