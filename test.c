#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "jsooooooon.h"

static int main_ret = 0;
static int test_count = 0;
static int test_pass = 0;

#define EXPECT_EQ_BASE(equality, expect, actual, format)\
    do {\
        test_count++;\
        if (equality)\
            test_pass++;\
        else {\
            fprintf(stderr, "%s:%d: expect: "format" actual: "format".\n", __FILE__, __LINE__, expect, actual);\
            main_ret = 1;\
        }\
    } while (0)

#define EXPECT_EQ_INT(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%d")

#define EXPECT_EQ_DOUBLE(expect, actual) EXPECT_EQ_BASE((expect) == (actual), expect, actual, "%.17g%")

#define TEST_ERROR(error, json)\
    do {\
        json_value v;\
        v.type = JSON_NULL;\
        EXPECT_EQ_INT(error, json_parse(&v, json));\
        EXPECT_EQ_INT(JSON_NULL, json_get_type(&v));\
    } while (0)
        
#define TEST_NUMBER(expect, json)\
    do {\
        json_value v;\
        EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, json));\
        EXPECT_EQ_INT(JSON_NUMBER, json_get_type(&v));\
        EXPECT_EQ_DOUBLE(expect, json_get_number(&v));\
    } while (0)

static void test_parse_expect_value() {
    TEST_ERROR(JSON_PARSE_EXPECT_VALUE, "");
    TEST_ERROR(JSON_PARSE_EXPECT_VALUE, " ");
}

static void test_parse_null() {
    json_value v;
    v.type = JSON_NULL;
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "null"));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&v));
}

static void test_parse_true() {
    json_value v;
    v.type = JSON_NULL;
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "true"));
    EXPECT_EQ_INT(JSON_TRUE, json_get_type(&v));
}

static void test_parse_false() {
    json_value v;
    v.type = JSON_NULL;
    EXPECT_EQ_INT(JSON_PARSE_OK, json_parse(&v, "false"));
    EXPECT_EQ_INT(JSON_FALSE, json_get_type(&v));
}

static void test_parse_root_not_singular() {
    json_value v;
    v.type = JSON_NULL;
    EXPECT_EQ_INT(JSON_PARSE_ROOT_NOT_SINGULAR, json_parse(&v, "null x"));
    EXPECT_EQ_INT(JSON_NULL, json_get_type(&v));
}

static void test_parse_number() {
    TEST_NUMBER(0.0, "0");
    TEST_NUMBER(0.0, "-0");
    TEST_NUMBER(0.0, "-0.0");
    TEST_NUMBER(1.0, "1");
    TEST_NUMBER(-1.0, "-1");
    TEST_NUMBER(1.5, "1.5");
    TEST_NUMBER(-1.5, "-1.5");
    TEST_NUMBER(3.1416, "3.1416");
    TEST_NUMBER(1E10, "1E10");
    TEST_NUMBER(1e10, "1e10");
    TEST_NUMBER(1E+10, "1E+10");
    TEST_NUMBER(1E-10, "1E-10");
    TEST_NUMBER(-1E10, "-1E10");
    TEST_NUMBER(-1e10, "-1e10");
    TEST_NUMBER(-1E+10, "-1E+10");
    TEST_NUMBER(-1E-10, "-1E-10");
    TEST_NUMBER(1.234E+10, "1.234E+10");
    TEST_NUMBER(1.234E-10, "1.234E-10");
    TEST_NUMBER(0.0, "1e-10000");
}

static void test_number_overflow() {
    TEST_ERROR(JSON_PARSE_NUMBER_OVERFLOW, "1e309");
    TEST_ERROR(JSON_PARSE_NUMBER_OVERFLOW, "-1e309");
}

static void test_parse_invalid_value() {
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "+0");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "+1");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, ".123");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "1."); 
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "INF");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "inf");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "NAN");
    TEST_ERROR(JSON_PARSE_INVALID_VALUE, "nan");
}

static void test_parse() {
    test_parse_null();
    test_parse_true();
    test_parse_false();
    test_parse_root_not_singular();
    test_parse_number();
    test_parse_invalid_value();
    test_number_overflow();
}

int main(void) {
    test_parse();
    printf("%d/%d (%3.2f%%) passed\n", test_pass, test_count, test_pass * 100.0 / test_count);
    return main_ret;
}

