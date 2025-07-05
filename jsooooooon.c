#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include "jsooooooon.h"

#define EXPECT(c, ch)\
    do {\
        assert(*c->json == (ch));\
        c->json++;\
    } while(0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
} json_context;

json_type json_get_type(const json_value *v) {
    assert(v != NULL);
    return v->type;
}

double json_get_number(const json_value *v) {
    assert(v != NULL && v->type == JSON_NUMBER);
    return v->n;
}

static void json_parse_whitespace(json_context *c) {
    const char *p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    c->json = p;
}

static int json_parse_literal(json_context *c, json_value *v, const char *literal, json_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return JSON_PARSE_INVALID_VALUE;
    c->json += 1;
    v->type = type;
    return JSON_PARSE_OK;
}

static int json_parse_number(json_context *c, json_value *v) {
    const char *p = c->json;

    // optional '-'
    int sign = 1;
    if (*p == '-') {
        sign = -1;
        p++;
    }

    // integer part
    if (*p == '0') {
        p++;
        // leading zeros not allowed unless it's just "0"
        if (ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
    } else if (ISDIGIT(*p)) {
        while (ISDIGIT(*p)) p++;
    } else {
        return JSON_PARSE_INVALID_VALUE; // must start with digit
    }

    // fractional part
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE; // must have digits after '.', not allowed 12.e
        // if (!ISDIGIT(*p) && *p != 'e' && *p != 'E') return JSON_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) p++;
    }

    // exponent part
    if (*p == 'e' || *p == 'E') {
        p++;
        if (*p == '+' || *p == '-') p++;
        if (!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE;
        while (ISDIGIT(*p)) p++;
    }
    
    errno = 0;
    v->n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->n == HUGE_VAL || v->n == -HUGE_VAL))
        return JSON_PARSE_NUMBER_OVERFLOW;

    v->type = JSON_NUMBER;
    c->json = p;
    return JSON_PARSE_OK;
}

static int json_parse_value(json_context *c, json_value *v) {
    switch (*c->json) {
        case 'n': return json_parse_literal(c, v, "null", JSON_NULL);
        case 't': return json_parse_literal(c, v, "true", JSON_TRUE);
        case 'f': return json_parse_literal(c, v, "false", JSON_FALSE);
        case '\0': return JSON_PARSE_EXPECT_VALUE;
        default: return json_parse_number(c, v);
    }
}

int json_parse(json_value *v, const char *json) {
    json_context c;
    assert(v != NULL);
    c.json = json;
    v->type = JSON_NULL;
    json_parse_whitespace(&c);
    return json_parse_value(&c, v);
}
