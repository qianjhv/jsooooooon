#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <math.h>
#include <string.h>

#include "jsooooooon.h"

#define EXPECT(c, ch) do { assert(*c->json == (ch)); c->json++; } while(0)

#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')
#define PUTC(c, ch) do { *(char*)json_context_push(c, sizeof(char)) = (ch); } while(0)

#ifndef JSON_PARSE_STACK_INIT_SIZE
#define JSON_PARSE_STACK_INIT_SIZE 256
#endif

typedef struct {
    const char* json;
    char* stack;
    size_t size, top;
} json_context;

static void* json_context_push(json_context* c, size_t size) {
    void* ret;
    assert(size > 0);

    if (c->top + size >= c->size) {
        if (size == 0)
            c->size = JSON_PARSE_STACK_INIT_SIZE;
        while (c->top + size >= c->size)
            c->size += c->size >> 1;
        c->stack = (char*)realloc(c->stack, c->size);
    }

    ret = c->stack + c->top;
    c->top += size;
    return ret;
}

static void* json_context_pop(json_context* c, size_t size) {
    assert(c->top >= size);
    return c->stack + (c->top -= size);
}

json_type json_get_type(const json_value* v) {
    assert(v != NULL);
    return v->type;
}

static void json_parse_whitespace(json_context* c) {
    const char* p = c->json;
    while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r') {
        p++;
    }
    c->json = p;
}

static int json_parse_literal(json_context* c, json_value* v, const char* literal, json_type type) {
    size_t i;
    EXPECT(c, literal[0]);
    for (i = 0; literal[i + 1]; i++)
        if (c->json[i] != literal[i + 1])
            return JSON_PARSE_INVALID_VALUE;
    c->json += 1;
    v->type = type;
    return JSON_PARSE_OK;
}

static int json_parse_number(json_context* c, json_value* v) {
    const char* p = c->json;

    // optional '-'
    if (*p == '-') p++;

    // integer part
    if (*p == '0') {
        p++;
        if (ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE; // leading zeros not allowed unless it's just "0"
    } else if (ISDIGIT(*p)) {
        while (ISDIGIT(*p)) p++;
    } else {
        return JSON_PARSE_INVALID_VALUE; // must start with digit
    }

    // fractional part
    if (*p == '.') {
        p++;
        if (!ISDIGIT(*p)) return JSON_PARSE_INVALID_VALUE; // must have digits after '.', not allowed 12.e
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
    v->u.n = strtod(c->json, NULL);
    if (errno == ERANGE && (v->u.n == HUGE_VAL || v->u.n == -HUGE_VAL))
        return JSON_PARSE_NUMBER_OVERFLOW;

    v->type = JSON_NUMBER;
    c->json = p;
    return JSON_PARSE_OK;
}

static int json_parse_string(json_context*c, json_value* v) {
    size_t head = c->top, len;
    const char* p;
    EXPECT(c, '\"');
    p = c->json;
    for (;;) {
        char ch = *p++;
        switch (ch) {
            case '\"':
                len = c->top - head;
                json_set_string(v, (const char*)json_context_pop(c, len), len);
                c->json = p;
                return JSON_PARSE_OK;
            case '\\':
                switch (*p++) {
                    case '\"': PUTC(c, '\"'); break;
                    case '\\': PUTC(c, '\\'); break;
                    case '/':  PUTC(c, '/' ); break;
                    case 'b':  PUTC(c, '\b'); break;
                    case 'f':  PUTC(c, '\f'); break;
                    case 'n':  PUTC(c, '\n'); break;
                    case 'r':  PUTC(c, '\r'); break;
                    case 't':  PUTC(c, '\t'); break;
                    default:
                        c->top = head;
                        return JSON_PARSE_INVALID_STRING_ESCAPE;
                }
                break;
            case '\0':
                c->top = head;
                return JSON_PARSE_MISS_QUOTATION_MARK;
            default:
                if ((unsigned char)ch < 0x20) { 
                    c->top = head;
                    return JSON_PARSE_INVALID_STRING_ESCAPE;
                }
                PUTC(c, ch);
        }
    }
} 

static int json_parse_value(json_context* c, json_value* v) {
    switch (*c->json) {
        case 'n': return json_parse_literal(c, v, "null", JSON_NULL);
        case 't': return json_parse_literal(c, v, "true", JSON_TRUE);
        case 'f': return json_parse_literal(c, v, "false", JSON_FALSE);
        case '\0': return JSON_PARSE_EXPECT_VALUE;
        case '"': return json_parse_string(c, v);
        default: return json_parse_number(c, v);
    }
}

int json_parse(json_value* v, const char* json) {
    int ret;
    json_context c;
    assert(v != NULL);
    c.json = json;
    c.stack = NULL;
    c.size = c.top = 0;
    json_init(v);
    v->type = JSON_NULL;
    json_parse_whitespace(&c);

    if ((ret = json_parse_value(&c, v)) == JSON_PARSE_OK) {
    }

    assert(c.top == 0);
    free(c.stack);
    return ret;
}

void json_free(json_value* v) {
    assert(v != NULL);
    if (v->type == JSON_STRING)
        free(v->u.s.s);
    v->type = JSON_NULL;
}

int json_get_boolean(const json_value* v) {
    assert(v != NULL && (v->type == JSON_TRUE || v->type == JSON_FALSE));
    return v->type == JSON_TRUE;
}

void json_set_boolean(json_value* v, int b) {
    json_free(v);
    v->type = b ? JSON_TRUE : JSON_FALSE;
}

double json_get_number(const json_value* v) {
    assert(v != NULL && v->type == JSON_NUMBER);
    return v->u.n;
}

void json_set_number(json_value* v, double n) {
    json_free(v);
    v->u.n = n;
    v->type = JSON_NUMBER;
}

const char* json_get_string(const json_value* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.s;
}

size_t json_get_string_length(const json_value* v) {
    assert(v != NULL && v->type == JSON_STRING);
    return v->u.s.len;
}

void json_set_string(json_value* v, const char* s, size_t len) {
    assert(v != NULL && (s != NULL || len == 0));
    json_free(v);
    v->u.s.s = (char*)malloc(len + 1);
    memcpy(v->u.s.s, s, len);
    v->u.s.s[len] = '\0';
    v->u.s.len = len;
    v->type = JSON_STRING;
}
