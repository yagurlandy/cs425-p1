#ifndef LAB_H
#define LAB_H
#include <stddef.h>
#include <sys/types.h>

/** * @brief Returns a greeting message.
 *
 * This function returns a string that contains a greeting message.
 * The string is allocated with malloc and should be freed by the caller.
 * @param name The name to include in the greeting.
 * @return A greeting string.
 */
char* get_greeting(const char* restrict name);

/**
 * @brief Gets the three-digit status code from an SMTP reply.
 *
 * @param line The SMTP reply line.
 * @return The status code, or -1 if the line is invalid.
 */
int smtp_parse_reply_code(const char *line);

/**
 * @brief Checks whether an SMTP reply line is the final line.
 *
 * @param line The SMTP reply line.
 * @return 1 if final, 0 if more lines follow, or -1 if invalid.
 */
int smtp_reply_is_final(const char *line);

/**
 * @brief Adds an extra dot to each line that starts with a dot.
 *
 * The returned string uses malloc and must be freed by the caller.
 *
 * @param message The email message.
 * @return The updated message, or NULL if memory could not be allocated.
 */
char *smtp_dot_stuff(const char *message);

/**
 * @brief Changes each message line ending to CRLF.
 *
 * The returned string uses malloc and must be freed by the caller.
 *
 * @param message The email message.
 * @return The updated message, or NULL if memory could not be allocated.
 */
char *smtp_normalize_crlf(const char *message);

/**
 * @brief Builds an SMTP command that ends with CRLF.
 *
 * The returned string uses malloc and must be freed by the caller.
 *
 * @param command The SMTP command.
 * @param argument The command argument, or NULL if there is no argument.
 * @return The completed command, or NULL if it could not be created.
 */
char *smtp_build_command(const char *command, const char *argument);

/**
 * @brief Builds the message sent after the SMTP DATA command.
 *
 * The returned string uses malloc and must be freed by the caller.
 *
 * @param from The sender's email address.
 * @param to The recipient's email address.
 * @param subject The email subject.
 * @param body The email body.
 * @return The completed message, or NULL if it could not be created.
 */
char *smtp_build_data_payload(const char *from, const char *to,
                              const char *subject, const char *body);

#define SMTP_READ_BUFFER_SIZE 1024

/* Callback functions for reading and writing SMTP data. */
typedef ssize_t (*smtp_read_callback)(void *context, void *buffer,
                                      size_t length);
typedef ssize_t (*smtp_write_callback)(void *context, const void *buffer,
                                       size_t length);

/* Stores data that has been read but not used yet. */
typedef struct
{
  smtp_read_callback read_callback;
  void *context;
  char buffer[SMTP_READ_BUFFER_SIZE];
  size_t start;
  size_t end;
} smtp_reader;

/* Sets up the reply reader. */
void smtp_reader_init(smtp_reader *reader,
                      smtp_read_callback read_callback,
                      void *context);

/* Reads one complete SMTP reply and returns its status code. */
int smtp_read_reply(smtp_reader *reader, char *reply, size_t reply_size);

/* Writes all of the provided SMTP data. */
int smtp_write_all(smtp_write_callback write_callback,
                   void *context,
                   const char *data,
                   size_t length);

/* Sends one SMTP command and checks the server's reply code. */
int smtp_send_command(smtp_reader *reader,
                      smtp_write_callback write_callback,
                      const char *command,
                      const char *argument,
                      int expected_code);

/* Runs the complete SMTP conversation. */
int smtp_run_session(smtp_read_callback read_callback,
                     smtp_write_callback write_callback,
                     void *context,
                     const char *helo_host,
                     const char *from,
                     const char *to,
                     const char *subject,
                     const char *body);

#endif // LAB_H
