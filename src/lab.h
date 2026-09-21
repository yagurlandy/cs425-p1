#ifndef LAB_H
#define LAB_H

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

#endif // LAB_H
