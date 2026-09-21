#include "lab.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *get_greeting(const char *restrict name)
{
  if (name == NULL)
  {
    return NULL;
  }

  // Allocate memory for the greeting message
  int length = snprintf(NULL, 0, "Hello, %s!", name);
  if (length < 0) // GCOVR_EXCL_START
  {
    return NULL; // snprintf failed
  } // GCOVR_EXCL_STOP

  //Casting is safe here because we know length is non-negative
  size_t alloc_size = (size_t) length + 1; // +1 for the null terminator
  char *greeting = malloc( alloc_size);


  if (greeting == NULL) // GCOVR_EXCL_START
  {
    return NULL; // Memory allocation failed
  }  // GCOVR_EXCL_STOP


  // Create the greeting message
  snprintf(greeting, alloc_size, "Hello, %s!", name);

  return greeting;
}

int smtp_parse_reply_code(const char *line)
{
  if (line == NULL)
  {
    return -1;
  }

  size_t length = strlen(line);

  if (length < 3)
  {
    return -1;
  }

  if (line[0] < '0' || line[0] > '9' ||
      line[1] < '0' || line[1] > '9' ||
      line[2] < '0' || line[2] > '9')
  {
    return -1;
  }

  if (length > 3 &&
      line[3] != ' ' &&
      line[3] != '-' &&
      line[3] != '\r')
  {
    return -1;
  }

  if (line[3] == '\r' &&
      (line[4] != '\n' || line[5] != '\0'))
  {
    return -1;
  }

  return ((line[0] - '0') * 100) +
         ((line[1] - '0') * 10) +
         (line[2] - '0');
}

int smtp_reply_is_final(const char *line)
{
  if (smtp_parse_reply_code(line) == -1)
  {
    return -1;
  }

  if (line[3] == '-')
  {
    return 0;
  }

  return 1;
}

char *smtp_dot_stuff(const char *message)
{
  if (message == NULL)
  {
    return NULL;
  }

  size_t length = strlen(message);
  size_t extra_dots = 0;
  int start_of_line = 1;

  for (size_t i = 0; i < length; i++)
  {
    if (start_of_line && message[i] == '.')
    {
      extra_dots++;
    }

    start_of_line = message[i] == '\n';
  }

  char *new_message = malloc(length + extra_dots + 1);
  if (new_message == NULL)
  { // GCOVR_EXCL_START
    return NULL;
  } // GCOVR_EXCL_STOP

  size_t new_index = 0;
  start_of_line = 1;

  for (size_t i = 0; i < length; i++)
  {
    if (start_of_line && message[i] == '.')
    {
      new_message[new_index] = '.';
      new_index++;
    }

    new_message[new_index] = message[i];
    new_index++;
    start_of_line = message[i] == '\n';
  }

  new_message[new_index] = '\0';
  return new_message;
}

char *smtp_normalize_crlf(const char *message)
{
  if (message == NULL)
  {
    return NULL;
  }

  size_t new_length = 0;

  for (size_t i = 0; message[i] != '\0'; i++)
  {
    if (message[i] == '\r')
    {
      new_length += 2;

      if (message[i + 1] == '\n')
      {
        i++;
      }
    }
    else if (message[i] == '\n')
    {
      new_length += 2;
    }
    else
    {
      new_length++;
    }
  }

  char *new_message = malloc(new_length + 1);
  if (new_message == NULL)
  { // GCOVR_EXCL_START
    return NULL;
  } // GCOVR_EXCL_STOP

  size_t new_index = 0;

  for (size_t i = 0; message[i] != '\0'; i++)
  {
    if (message[i] == '\r')
    {
      new_message[new_index++] = '\r';
      new_message[new_index++] = '\n';

      if (message[i + 1] == '\n')
      {
        i++;
      }
    }
    else if (message[i] == '\n')
    {
      new_message[new_index++] = '\r';
      new_message[new_index++] = '\n';
    }
    else
    {
      new_message[new_index++] = message[i];
    }
  }

  new_message[new_index] = '\0';
  return new_message;
}

char *smtp_build_command(const char *command, const char *argument)
{
  if (command == NULL || command[0] == '\0')
  {
    return NULL;
  }

  if (strchr(command, '\r') != NULL || strchr(command, '\n') != NULL)
  {
    return NULL;
  }

  if (argument != NULL &&
      (strchr(argument, '\r') != NULL || strchr(argument, '\n') != NULL))
  {
    return NULL;
  }

  size_t command_length = strlen(command);
  size_t argument_length = 0;

  if (argument != NULL)
  {
    argument_length = strlen(argument);
  }

  size_t total_length = command_length + argument_length + 3;

  if (argument_length > 0)
  {
    total_length++;
  }

  char *result = malloc(total_length);
  if (result == NULL)
  { // GCOVR_EXCL_START
    return NULL;
  } // GCOVR_EXCL_STOP

  if (argument_length > 0)
  {
    snprintf(result, total_length, "%s %s\r\n", command, argument);
  }
  else
  {
    snprintf(result, total_length, "%s\r\n", command);
  }

  return result;
}

char *smtp_build_data_payload(const char *from, const char *to,
                              const char *subject, const char *body)
{
  if (from == NULL || to == NULL || subject == NULL || body == NULL)
  {
    return NULL;
  }

  if (strchr(from, '\r') != NULL || strchr(from, '\n') != NULL ||
      strchr(to, '\r') != NULL || strchr(to, '\n') != NULL ||
      strchr(subject, '\r') != NULL || strchr(subject, '\n') != NULL)
  {
    return NULL;
  }

  char *updated_body = smtp_normalize_crlf(body);
  if (updated_body == NULL)
  { // GCOVR_EXCL_START
    return NULL;
  } // GCOVR_EXCL_STOP

  char *stuffed_body = smtp_dot_stuff(updated_body);
  free(updated_body);

  if (stuffed_body == NULL)
  { // GCOVR_EXCL_START
    return NULL;
  } // GCOVR_EXCL_STOP

  size_t body_length = strlen(stuffed_body);
  int needs_crlf = body_length < 2 ||
                   stuffed_body[body_length - 2] != '\r' ||
                   stuffed_body[body_length - 1] != '\n';

  int needed;

  if (needs_crlf)
  {
    needed = snprintf(NULL, 0,
                      "From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n%s\r\n.\r\n",
                      from, to, subject, stuffed_body);
  }
  else
  {
    needed = snprintf(NULL, 0,
                      "From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n%s.\r\n",
                      from, to, subject, stuffed_body);
  }

  if (needed < 0)
  { // GCOVR_EXCL_START
    free(stuffed_body);
    return NULL;
  } // GCOVR_EXCL_STOP

  size_t result_size = (size_t)needed + 1;
  char *result = malloc(result_size);

  if (result == NULL)
  { // GCOVR_EXCL_START
    free(stuffed_body);
    return NULL;
  } // GCOVR_EXCL_STOP

  if (needs_crlf)
  {
    snprintf(result, result_size,
             "From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n%s\r\n.\r\n",
             from, to, subject, stuffed_body);
  }
  else
  {
    snprintf(result, result_size,
             "From: %s\r\nTo: %s\r\nSubject: %s\r\n\r\n%s.\r\n",
             from, to, subject, stuffed_body);
  }

  free(stuffed_body);
  return result;
}

void smtp_reader_init(smtp_reader *reader,
                      smtp_read_callback read_callback,
                      void *context)
{
  if (reader == NULL)
  {
    return;
  }

  reader->read_callback = read_callback;
  reader->context = context;
  reader->start = 0;
  reader->end = 0;
}

static int smtp_read_line(smtp_reader *reader, char *line, size_t line_size)
{
  if (reader == NULL || reader->read_callback == NULL ||
      line == NULL || line_size == 0)
  {
    return -1;
  }

  size_t used = 0;
  line[0] = '\0';

  while (1)
  {
    if (reader->start == reader->end)
    {
      ssize_t amount = reader->read_callback(
          reader->context, reader->buffer, sizeof(reader->buffer));

      if (amount <= 0 || (size_t)amount > sizeof(reader->buffer))
      {
        return -1;
      }

      reader->start = 0;
      reader->end = (size_t)amount;
    }

    if (used + 1 >= line_size)
    {
      return -1;
    }

    line[used] = reader->buffer[reader->start];
    used++;
    reader->start++;
    line[used] = '\0';

    if (used >= 2 && line[used - 2] == '\r' &&
        line[used - 1] == '\n')
    {
      return 0;
    }
  }
}

int smtp_read_reply(smtp_reader *reader, char *reply, size_t reply_size)
{
  if (reader == NULL || reply == NULL || reply_size == 0)
  {
    return -1;
  }

  char line[SMTP_READ_BUFFER_SIZE];
  size_t reply_length = 0;
  int expected_code = -1;

  reply[0] = '\0';

  while (1)
  {
    if (smtp_read_line(reader, line, sizeof(line)) != 0)
    {
      return -1;
    }

    int code = smtp_parse_reply_code(line);
    int is_final = smtp_reply_is_final(line);

    if (code < 0 || is_final < 0)
    {
      return -1;
    }

    if (expected_code < 0)
    {
      expected_code = code;
    }
    else if (code != expected_code)
    {
      return -1;
    }

    size_t line_length = strlen(line);

    if (reply_length + line_length + 1 > reply_size)
    {
      return -1;
    }

    memcpy(reply + reply_length, line, line_length);
    reply_length += line_length;
    reply[reply_length] = '\0';

    if (is_final)
    {
      return code;
    }
  }
}

int smtp_write_all(smtp_write_callback write_callback,
                   void *context,
                   const char *data,
                   size_t length)
{
  if (write_callback == NULL || data == NULL)
  {
    return -1;
  }

  size_t written = 0;

  while (written < length)
  {
    ssize_t amount = write_callback(
        context, data + written, length - written);

    if (amount <= 0 || (size_t)amount > length - written)
    {
      return -1;
    }

    written += (size_t)amount;
  }

  return 0;
}

static void smtp_report_reply_error(const char *step,
                                    int expected_code,
                                    int received_code)
{
#ifndef TEST
  if (received_code < 0)
  {
    fprintf(stderr, "Could not read the SMTP reply after %s.\n", step);
  }
  else
  {
    fprintf(stderr,
            "Unexpected SMTP reply after %s: expected %d, received %d.\n",
            step, expected_code, received_code);
  }
#else
  (void)step;
  (void)expected_code;
  (void)received_code;
#endif
}

int smtp_send_command(smtp_reader *reader,
                      smtp_write_callback write_callback,
                      const char *command,
                      const char *argument,
                      int expected_code)
{
  if (reader == NULL || write_callback == NULL)
  {
    return -1;
  }

  char *command_line = smtp_build_command(command, argument);
  if (command_line == NULL)
  {
    return -1;
  }

  int write_result = smtp_write_all(
      write_callback,
      reader->context,
      command_line,
      strlen(command_line));

  free(command_line);

  if (write_result != 0)
  {
    return -1;
  }

  char reply[4096];
  int reply_code = smtp_read_reply(reader, reply, sizeof(reply));

  if (reply_code != expected_code)
  {
    smtp_report_reply_error(command, expected_code, reply_code);
    return -1;
  }

  return 0;
}

int smtp_run_session(smtp_read_callback read_callback,
                     smtp_write_callback write_callback,
                     void *context,
                     const char *helo_host,
                     const char *from,
                     const char *to,
                     const char *subject,
                     const char *body)
{
  if (read_callback == NULL || write_callback == NULL ||
      helo_host == NULL || from == NULL || to == NULL ||
      subject == NULL || body == NULL)
  {
    return -1;
  }

  char *payload = smtp_build_data_payload(from, to, subject, body);
  if (payload == NULL)
  {
    return -1;
  }

  size_t mail_size = strlen(from) + 8;
  char *mail_argument = malloc(mail_size);

  if (mail_argument == NULL)
  { // GCOVR_EXCL_START
    free(payload);
    return -1;
  } // GCOVR_EXCL_STOP

  snprintf(mail_argument, mail_size, "FROM:<%s>", from);

  size_t rcpt_size = strlen(to) + 6;
  char *rcpt_argument = malloc(rcpt_size);

  if (rcpt_argument == NULL)
  { // GCOVR_EXCL_START
    free(mail_argument);
    free(payload);
    return -1;
  } // GCOVR_EXCL_STOP

  snprintf(rcpt_argument, rcpt_size, "TO:<%s>", to);

  smtp_reader reader;
  smtp_reader_init(&reader, read_callback, context);

  char reply[4096];
  int result = -1;

  int greeting_code = smtp_read_reply(&reader, reply, sizeof(reply));

  if (greeting_code != 220)
  {
    smtp_report_reply_error("server greeting", 220, greeting_code);
    goto cleanup;
  }

  if (smtp_send_command(
          &reader, write_callback, "HELO", helo_host, 250) != 0)
  {
    goto cleanup;
  }

  if (smtp_send_command(
          &reader, write_callback, "MAIL", mail_argument, 250) != 0)
  {
    goto cleanup;
  }

  if (smtp_send_command(
          &reader, write_callback, "RCPT", rcpt_argument, 250) != 0)
  {
    goto cleanup;
  }

  if (smtp_send_command(
          &reader, write_callback, "DATA", NULL, 354) != 0)
  {
    goto cleanup;
  }

  if (smtp_write_all(
          write_callback, context, payload, strlen(payload)) != 0)
  {
    goto cleanup;
  }

  int message_code = smtp_read_reply(&reader, reply, sizeof(reply));

  if (message_code != 250)
  {
    smtp_report_reply_error("message data", 250, message_code);
    goto cleanup;
  }

  if (smtp_send_command(
          &reader, write_callback, "QUIT", NULL, 221) != 0)
  {
    goto cleanup;
  }

  result = 0;

cleanup:
  free(rcpt_argument);
  free(mail_argument);
  free(payload);
  return result;
}