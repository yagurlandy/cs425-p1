#include <stdlib.h>
#include <stdio.h>
#include "harness/unity.h"
#include "../src/lab.h"
#include <string.h>

typedef struct
{
  const char *data;
  size_t position;
  size_t chunk_size;
} fake_read_data;

static ssize_t fake_read(void *context, void *buffer, size_t length)
{
  fake_read_data *fake = context;
  size_t data_length = strlen(fake->data);

  if (fake->position >= data_length)
  {
    return 0;
  }

  size_t amount = data_length - fake->position;

  if (amount > fake->chunk_size)
  {
    amount = fake->chunk_size;
  }

  if (amount > length)
  {
    amount = length;
  }

  memcpy(buffer, fake->data + fake->position, amount);
  fake->position += amount;

  return (ssize_t)amount;
}

typedef struct
{
  char data[512];
  size_t length;
  size_t chunk_size;
} fake_write_data;

static ssize_t fake_write(void *context, const void *buffer, size_t length)
{
  fake_write_data *fake = context;
  size_t amount = length;

  if (amount > fake->chunk_size)
  {
    amount = fake->chunk_size;
  }

  if (fake->length + amount > sizeof(fake->data))
  {
    return -1;
  }

  memcpy(fake->data + fake->length, buffer, amount);
  fake->length += amount;

  return (ssize_t)amount;
}

static ssize_t fake_write_failure(void *context,
                                  const void *buffer,
                                  size_t length)
{
  (void)context;
  (void)buffer;
  (void)length;
  return 0;
}

static ssize_t fake_write_too_much(void *context,
                                   const void *buffer,
                                   size_t length)
{
  (void)context;
  (void)buffer;
  return (ssize_t)length + 1;
}

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

typedef struct
{
  fake_read_data input;
  fake_write_data output;
} fake_connection;

static ssize_t fake_connection_read(void *context,
                                    void *buffer,
                                    size_t length)
{
  fake_connection *connection = context;
  return fake_read(&connection->input, buffer, length);
}

static ssize_t fake_connection_write(void *context,
                                     const void *buffer,
                                     size_t length)
{
  fake_connection *connection = context;
  return fake_write(&connection->output, buffer, length);
}

static ssize_t fake_payload_write_failure(void *context,
                                          const void *buffer,
                                          size_t length)
{
  if (length >= 5 && memcmp(buffer, "From:", 5) == 0)
  {
    return -1;
  }

  return fake_connection_write(context, buffer, length);
}

static int run_fake_session(fake_connection *connection)
{
  return smtp_run_session(
      fake_connection_read,
      fake_connection_write,
      connection,
      "localhost",
      "sender@example.com",
      "receiver@example.com",
      "Test",
      "Hello\n.World");
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
  TEST_ASSERT_EQUAL_INT(250, smtp_parse_reply_code("250\r\n"));
  TEST_ASSERT_EQUAL_INT(250, smtp_parse_reply_code("250"));
  TEST_ASSERT_EQUAL_INT(-1, smtp_parse_reply_code("250\rbad"));
}

void test_smtp_reply_is_final(void)
{
  TEST_ASSERT_EQUAL_INT(1, smtp_reply_is_final("250 OK\r\n"));
  TEST_ASSERT_EQUAL_INT(0, smtp_reply_is_final("250-first line\r\n"));

  TEST_ASSERT_EQUAL_INT(-1, smtp_reply_is_final(NULL));
  TEST_ASSERT_EQUAL_INT(-1, smtp_reply_is_final("invalid\r\n"));

    TEST_ASSERT_EQUAL_INT(1, smtp_reply_is_final("250\r\n"));
  TEST_ASSERT_EQUAL_INT(1, smtp_reply_is_final("250"));
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

void test_smtp_normalize_crlf(void)
{
  char *result = smtp_normalize_crlf(
      "First\n"
      "Second\r"
      "Third\r\n"
      "Fourth");

  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING(
      "First\r\n"
      "Second\r\n"
      "Third\r\n"
      "Fourth",
      result);

  free(result);

  result = smtp_normalize_crlf("");
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("", result);
  free(result);

  TEST_ASSERT_NULL(smtp_normalize_crlf(NULL));
}

void test_smtp_build_command(void)
{
  char *result = smtp_build_command("HELO", "localhost");

  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("HELO localhost\r\n", result);
  free(result);

  result = smtp_build_command("DATA", NULL);
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("DATA\r\n", result);
  free(result);

  result = smtp_build_command("QUIT", "");
  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING("QUIT\r\n", result);
  free(result);

  TEST_ASSERT_NULL(smtp_build_command(NULL, NULL));
  TEST_ASSERT_NULL(smtp_build_command("", NULL));
  TEST_ASSERT_NULL(smtp_build_command("HELO\r", "localhost"));
  TEST_ASSERT_NULL(smtp_build_command("HELO\n", "localhost"));
  TEST_ASSERT_NULL(smtp_build_command("HELO", "local\rhost"));
  TEST_ASSERT_NULL(smtp_build_command("HELO", "local\nhost"));
}

void test_smtp_build_data_payload(void)
{
  char *result = smtp_build_data_payload(
      "sender@example.com",
      "receiver@example.com",
      "Test subject",
      ".First line\nSecond line");

  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING(
      "From: sender@example.com\r\n"
      "To: receiver@example.com\r\n"
      "Subject: Test subject\r\n"
      "\r\n"
      "..First line\r\n"
      "Second line\r\n"
      ".\r\n",
      result);
  free(result);

  result = smtp_build_data_payload(
      "sender@example.com",
      "receiver@example.com",
      "",
      "Already finished\r\n");

  TEST_ASSERT_NOT_NULL(result);
  TEST_ASSERT_EQUAL_STRING(
      "From: sender@example.com\r\n"
      "To: receiver@example.com\r\n"
      "Subject: \r\n"
      "\r\n"
      "Already finished\r\n"
      ".\r\n",
      result);
  free(result);

  TEST_ASSERT_NULL(smtp_build_data_payload(
      NULL, "receiver@example.com", "Subject", "Body"));
  TEST_ASSERT_NULL(smtp_build_data_payload(
      "sender@example.com", NULL, "Subject", "Body"));
  TEST_ASSERT_NULL(smtp_build_data_payload(
      "sender@example.com", "receiver@example.com", NULL, "Body"));
  TEST_ASSERT_NULL(smtp_build_data_payload(
      "sender@example.com", "receiver@example.com", "Subject", NULL));

  TEST_ASSERT_NULL(smtp_build_data_payload(
      "sender\n@example.com", "receiver@example.com", "Subject", "Body"));
  TEST_ASSERT_NULL(smtp_build_data_payload(
      "sender@example.com", "receiver\r@example.com", "Subject", "Body"));
  TEST_ASSERT_NULL(smtp_build_data_payload(
      "sender@example.com", "receiver@example.com", "Bad\nsubject", "Body"));
}

void test_smtp_read_reply(void)
{
  smtp_reader reader;
  char reply[128];

  fake_read_data normal = {
      "220 server ready\r\n",
      0,
      2
  };

  smtp_reader_init(&reader, fake_read, &normal);

  TEST_ASSERT_EQUAL_INT(220,
                        smtp_read_reply(&reader, reply, sizeof(reply)));
  TEST_ASSERT_EQUAL_STRING("220 server ready\r\n", reply);

  fake_read_data multiline = {
      "250-first line\r\n"
      "250 second line\r\n"
      "221 goodbye\r\n",
      0,
      100
  };

  smtp_reader_init(&reader, fake_read, &multiline);

  TEST_ASSERT_EQUAL_INT(250,
                        smtp_read_reply(&reader, reply, sizeof(reply)));
  TEST_ASSERT_EQUAL_STRING(
      "250-first line\r\n250 second line\r\n",
      reply);

  TEST_ASSERT_EQUAL_INT(221,
                        smtp_read_reply(&reader, reply, sizeof(reply)));
  TEST_ASSERT_EQUAL_STRING("221 goodbye\r\n", reply);

  fake_read_data small = {
      "250 okay\r\n",
      0,
      100
  };

  smtp_reader_init(&reader, fake_read, &small);

  char small_reply[5];
  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, small_reply, sizeof(small_reply)));

  fake_read_data hangup = {
      "250 unfinished",
      0,
      3
  };

  smtp_reader_init(&reader, fake_read, &hangup);

  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, reply, sizeof(reply)));

  fake_read_data invalid = {
      "not an SMTP reply\r\n",
      0,
      100
  };

  smtp_reader_init(&reader, fake_read, &invalid);

  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, reply, sizeof(reply)));

  fake_read_data mismatch = {
      "250-first line\r\n"
      "251 wrong code\r\n",
      0,
      100
  };

  smtp_reader_init(&reader, fake_read, &mismatch);

  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, reply, sizeof(reply)));

    char long_line[SMTP_READ_BUFFER_SIZE + 1];
  memset(long_line, 'A', SMTP_READ_BUFFER_SIZE);
  long_line[SMTP_READ_BUFFER_SIZE] = '\0';

  fake_read_data too_long = {
      long_line,
      0,
      100
  };

  smtp_reader_init(&reader, fake_read, &too_long);

  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, reply, sizeof(reply)));

  smtp_reader_init(NULL, fake_read, &normal);
  smtp_reader_init(&reader, NULL, NULL);

  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, reply, sizeof(reply)));
  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(NULL, reply, sizeof(reply)));
  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, NULL, sizeof(reply)));
  TEST_ASSERT_EQUAL_INT(
      -1, smtp_read_reply(&reader, reply, 0));
}

void test_smtp_write_all(void)
{
  fake_write_data fake = {
      "",
      0,
      3
  };

  const char *command = "HELO localhost\r\n";

  TEST_ASSERT_EQUAL_INT(
      0,
      smtp_write_all(fake_write, &fake, command, strlen(command)));

  fake.data[fake.length] = '\0';
  TEST_ASSERT_EQUAL_STRING(command, fake.data);

  TEST_ASSERT_EQUAL_INT(
      0, smtp_write_all(fake_write, &fake, "", 0));

  TEST_ASSERT_EQUAL_INT(
      -1, smtp_write_all(NULL, &fake, command, strlen(command)));

  TEST_ASSERT_EQUAL_INT(
      -1, smtp_write_all(fake_write, &fake, NULL, 0));

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_write_all(fake_write_failure,
                     NULL,
                     command,
                     strlen(command)));

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_write_all(fake_write_too_much,
                     NULL,
                     command,
                     strlen(command)));
}

void test_smtp_send_command(void)
{
  smtp_reader reader;

  fake_connection success = {
      {"250 hello\r\n", 0, 2},
      {"", 0, 3}
  };

  smtp_reader_init(&reader, fake_connection_read, &success);

  TEST_ASSERT_EQUAL_INT(
      0,
      smtp_send_command(
          &reader, fake_connection_write, "HELO", "localhost", 250));

  success.output.data[success.output.length] = '\0';
  TEST_ASSERT_EQUAL_STRING(
      "HELO localhost\r\n", success.output.data);

  fake_connection wrong_code = {
      {"500 command failed\r\n", 0, 100},
      {"", 0, 100}
  };

  smtp_reader_init(&reader, fake_connection_read, &wrong_code);

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_send_command(
          &reader, fake_connection_write, "HELO", "localhost", 250));

  fake_connection read_failure = {
      {"", 0, 100},
      {"", 0, 100}
  };

  smtp_reader_init(&reader, fake_connection_read, &read_failure);

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_send_command(
          &reader, fake_connection_write, "QUIT", NULL, 221));

  fake_connection write_failure = {
      {"250 okay\r\n", 0, 100},
      {"", 0, 100}
  };

  smtp_reader_init(&reader, fake_connection_read, &write_failure);

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_send_command(
          &reader, fake_write_failure, "HELO", "localhost", 250));

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_send_command(
          NULL, fake_connection_write, "HELO", "localhost", 250));

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_send_command(
          &reader, NULL, "HELO", "localhost", 250));

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_send_command(
          &reader, fake_connection_write, NULL, "localhost", 250));
}

void test_smtp_run_session(void)
{
  fake_connection success = {
      {
          "220 ready\r\n"
          "250 hello\r\n"
          "250 sender okay\r\n"
          "250 recipient okay\r\n"
          "354 start mail\r\n"
          "250 message accepted\r\n"
          "221 goodbye\r\n",
          0,
          3
      },
      {"", 0, 4}
  };

  TEST_ASSERT_EQUAL_INT(0, run_fake_session(&success));

  success.output.data[success.output.length] = '\0';

  TEST_ASSERT_EQUAL_STRING(
      "HELO localhost\r\n"
      "MAIL FROM:<sender@example.com>\r\n"
      "RCPT TO:<receiver@example.com>\r\n"
      "DATA\r\n"
      "From: sender@example.com\r\n"
      "To: receiver@example.com\r\n"
      "Subject: Test\r\n"
      "\r\n"
      "Hello\r\n"
      "..World\r\n"
      ".\r\n"
      "QUIT\r\n",
      success.output.data);

  const char *wrong_replies[] = {
      "500 wrong greeting\r\n",

      "220 ready\r\n"
      "500 wrong HELO reply\r\n",

      "220 ready\r\n"
      "250 hello\r\n"
      "500 wrong MAIL reply\r\n",

      "220 ready\r\n"
      "250 hello\r\n"
      "250 sender okay\r\n"
      "500 wrong RCPT reply\r\n",

      "220 ready\r\n"
      "250 hello\r\n"
      "250 sender okay\r\n"
      "250 recipient okay\r\n"
      "500 wrong DATA reply\r\n",

      "220 ready\r\n"
      "250 hello\r\n"
      "250 sender okay\r\n"
      "250 recipient okay\r\n"
      "354 start mail\r\n"
      "500 wrong message reply\r\n",

      "220 ready\r\n"
      "250 hello\r\n"
      "250 sender okay\r\n"
      "250 recipient okay\r\n"
      "354 start mail\r\n"
      "250 message accepted\r\n"
      "500 wrong QUIT reply\r\n"
  };

  size_t wrong_count = sizeof(wrong_replies) / sizeof(wrong_replies[0]);

  for (size_t i = 0; i < wrong_count; i++)
  {
    fake_connection failure = {
        {wrong_replies[i], 0, 5},
        {"", 0, 5}
    };

    TEST_ASSERT_EQUAL_INT(-1, run_fake_session(&failure));
  }

  fake_connection hangup = {
      {
          "220 ready\r\n"
          "250 hello\r\n",
          0,
          4
      },
      {"", 0, 4}
  };

  TEST_ASSERT_EQUAL_INT(-1, run_fake_session(&hangup));

  fake_connection write_failure = {
      {
          "220 ready\r\n",
          0,
          100
      },
      {"", 0, 100}
  };

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_run_session(
          fake_connection_read,
          fake_write_failure,
          &write_failure,
          "localhost",
          "sender@example.com",
          "receiver@example.com",
          "Test",
          "Body"));

    fake_connection payload_failure = {
      {
          "220 ready\r\n"
          "250 hello\r\n"
          "250 sender okay\r\n"
          "250 recipient okay\r\n"
          "354 start mail\r\n",
          0,
          100
      },
      {"", 0, 100}
  };

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_run_session(
          fake_connection_read,
          fake_payload_write_failure,
          &payload_failure,
          "localhost",
          "sender@example.com",
          "receiver@example.com",
          "Test",
          "Body"));

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_run_session(
          NULL,
          fake_connection_write,
          &success,
          "localhost",
          "sender@example.com",
          "receiver@example.com",
          "Test",
          "Body"));

  TEST_ASSERT_EQUAL_INT(
      -1,
      smtp_run_session(
          fake_connection_read,
          fake_connection_write,
          &success,
          "localhost",
          "sender@example.com",
          "receiver@example.com",
          "Bad\nsubject",
          "Body"));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_get_greeting);
  RUN_TEST(test_smtp_parse_reply_code);
  RUN_TEST(test_smtp_reply_is_final);
  RUN_TEST(test_smtp_dot_stuff);
  RUN_TEST(test_smtp_normalize_crlf);
  RUN_TEST(test_smtp_build_command);
  RUN_TEST(test_smtp_build_data_payload);
  RUN_TEST(test_smtp_read_reply);
  RUN_TEST(test_smtp_write_all);
  RUN_TEST(test_smtp_send_command);
  RUN_TEST(test_smtp_run_session);
  return UNITY_END();
}
