#include <stdlib.h>
#include <stdio.h>
#include "harness/unity.h"
#include "../src/lab.h"


void setUp(void) {
  printf("Setting up tests...\n");
}

void tearDown(void) {
  printf("Tearing down tests...\n");
}

void test_get_greeting(void) {
  char *greeting = get_greeting("Alice");
  TEST_ASSERT_NOT_NULL(greeting);
  TEST_ASSERT_EQUAL_STRING("Hello, Alice!", greeting);
  free(greeting); // Free the allocated memory for the greeting

  greeting = get_greeting(NULL);
  TEST_ASSERT_NULL(greeting);

  greeting = get_greeting("");
  TEST_ASSERT_NOT_NULL(greeting);
  TEST_ASSERT_EQUAL_STRING("Hello, !", greeting);
  free(greeting);
}

void test_smtp_parse_reply_code(void)
{
  TEST_ASSERT_EQUAL_INT(220, smtp_parse_reply_code("220 ready\r\n"));
  TEST_ASSERT_EQUAL_INT(250, smtp_parse_reply_code("250-more\r\n"));

  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code(NULL));
  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code(""));
  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code("22"));
  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code("x20 invalid\r\n"));
  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code("2x0 invalid\r\n"));
  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code("25x invalid\r\n"));
  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code("250_invalid\r\n"));
}

void test_smtp_reply_is_final(void)
{
  TEST_ASSERT_EQUAL_INT(1, smtp_reply_is_final("250 OK\r\n"));
  TEST_ASSERT_EQUAL_INT(0, smtp_reply_is_final("250-first line\r\n"));

  TEST_ASSERT_EQUAL_INT(-1, smtp_reply_is_final(NULL));
  TEST_ASSERT_EQUAL_INT(-1, smtp_reply_is_final("invalid\r\n"));
}

void test_smtp_dot_stuff(void)
{
  char *result = smtp_dot_stuff(
      ".First line\r\n"
      "Normal line\r\n"
      "..Two dots\r\n");

  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING(
      "..First line\r\n"
      "Normal line\r\n"
      "...Two dots\r\n",
      result);

  free(result);

  result = smtp_dot_stuff("");
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("", result);
  free(result);

  TEST_ASSERT_NULL(smtp_dot_stuff(NULL));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_get_greeting);
  RUN_TEST(test_smtp_parse_reply_code);
  RUN_TEST(test_smtp_reply_is_final);
  RUN_TEST(test_smtp_dot_stuff);
  return UNITY_END();
}
