# Project 1

* Name: Andy Lopez-Martinez
* Email: [andylopezmartine@u.boisestate.edu](mailto:andylopezmartine@u.boisestate.edu)
* Class: CS425-001

## Known Bugs or Issues

None at this time.

## Experience

The hardest part of this project was working with SMTP replies because one read does not always give the program a complete line. I used a buffer to save the data until a complete line ending in CRLF was received.

Testing each part separately helped me better understand how the SMTP commands, reply codes, message formatting, and socket connection all work together. Using callbacks also made it possible to test the SMTP conversation without needing to connect to a real mail server for every test.

## Analysis

I separated the program into three layers. The first layer contains the helper functions. These functions parse reply codes, check multiline replies, build commands, change line endings to CRLF, apply dot stuffing, and build the email message.

The second layer handles the SMTP conversation through read and write callbacks. It checks the server’s reply after each step and can handle data that is read or written in smaller pieces. The conversation sends HELO, MAIL FROM, RCPT TO, DATA, the email message, and QUIT.

The third layer is in `main.c`. It handles the command-line options with `getopt`, reads the message body from standard input when needed, and connects to the server with `getaddrinfo` and `connect`. It then uses `recv` and `send` with the callback layer.

The project currently has 11 tests and 100% line coverage for `src/lab.c`. The tests include multiline replies, replies received in small pieces, buffers that are too small, partial writes, connection failures, incorrect reply codes, CRLF conversion, and dot stuffing. GitHub Actions also runs the project checks after changes are pushed.
