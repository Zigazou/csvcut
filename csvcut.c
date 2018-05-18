#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>

/* Maximum number of columns csvcut can handle */
#define MAX_COLUMNS (128*1024)

/* Input and output buffer sizes */
#define BUFFER_SIZE 16384

/* Input buffer */
unsigned char buffer_in[BUFFER_SIZE];
unsigned int buffer_in_pos = 0;
unsigned int buffer_in_length = 0;

/* Output buffer */
unsigned char buffer_out[BUFFER_SIZE];
unsigned int buffer_out_length = 0;

/* Delimiter, is set by the user */
unsigned char delimiter = ',';

/* Column to extract, is set by the user */
#define COLUMN_SELECTED 1
#define COLUMN_IGNORED 0
unsigned int last_column = 0;
unsigned int first_column = 0;
unsigned char column_selects[MAX_COLUMNS];

/* The 3 possible states of the automaton */
#define STATE_DEFAULT 0
#define STATE_STRING 1
#define STATE_DOUBLE_QUOTE 2

/* Current state of the automaton */
unsigned int current_column = 0;
unsigned int current_state = STATE_DEFAULT;

/* Output a message on stderr and exit with a return code */
void error(const char *message, int rc) {
    fprintf(stderr, "CSVCUT: %s\n", message);
    exit(rc);
}

/* Buffered read of a char */
unsigned char readchar() {
    unsigned char c;

    /* Test if the input buffer has reached its end */
    if(buffer_in_pos >= buffer_in_length) {
        /* Load another chunks of data */
        buffer_in_length = read(0, buffer_in, sizeof(buffer_in));

        /* Test if data has been read */
        if(buffer_in_length == 0) {
            /* No more data to read, flush the output buffer */
            if(write(1, buffer_out, buffer_out_length) != buffer_out_length) {
                error("Unable to write on stdout", 5);
            }

            exit(0);
        }

        /* Reset the input buffer cursor */
        buffer_in_pos = 0;
    }

    /* Read a character from the buffer */
    c = buffer_in[buffer_in_pos];
    buffer_in_pos++;

    return c;
}

/* Buffered write of a char */
void writechar(unsigned char c) {
    /* Test if the output buffer is full */
    if(buffer_out_length == BUFFER_SIZE) {
        /* Flush the output buffer */
        if(write(1, buffer_out, buffer_out_length) != buffer_out_length) {
            /* Something went wrong, exit! */
            error("Unable to write on stdout", 5);
        }

        /* Reset the output buffer length */
        buffer_out_length = 0;
    }

    /* Write the character in the output buffer */
    buffer_out[buffer_out_length] = c;
    buffer_out_length++;
}

/* Handle character received when in default state */
void state_default(unsigned char c) {
    if(c == '"') {
        current_state = STATE_STRING;
    } else if(c == '\n') {
        current_column = 0;
    } else if(c == delimiter) {
        current_column++;
    }
}

/* Handle character received when in string state */
void state_string(unsigned char c) {
    if(c == '"') {
        current_state = STATE_DOUBLE_QUOTE;
    }
}

/* Handle character received after a double-quote */
void state_double_quote(unsigned char c) {
    if(c == '"') {
        current_state = STATE_STRING;
    } else if(c == '\n') {
        current_column = 0;
        current_state = STATE_DEFAULT;
    } else if(c == delimiter) {
        current_column++;
        current_state = STATE_DEFAULT;
    } else {
        current_state = STATE_DEFAULT;
    }
}

/* Tell if a character must be written to the output */
bool must_be_written(unsigned char c) {
    /* csvcut does not handle that much columns */
    if(current_column >= MAX_COLUMNS) {
        return false;
    }

    /* Output the content of a selected column */
    if(column_selects[current_column] == COLUMN_SELECTED) {
        /* The delimiter must be written except for the first selected column */
        if(c != delimiter || current_column > first_column) {
            return true;
        }
    }

    /* Output the line return of a new row */
    if(current_column == 0 && c == '\n') {
        return true;
    }

    return false;
}

/* The automaton that can read CSV data */
void automaton() {
    unsigned char c;

    while(true) {
        /* Read one character at a time */
        c = readchar();

        /* Execute actions according to the current automaton state */
        switch(current_state) {
            case STATE_DEFAULT: state_default(c); break;
            case STATE_STRING: state_string(c); break;
            case STATE_DOUBLE_QUOTE: state_double_quote(c); break;
            default:
                error("Unknown automaton state", 3);
        }        

        /* Write the character if it must be written */
        if(must_be_written(c)) {
           writechar(c);
        }
    }
}

int main(int argc, char *argv[]) {
    unsigned int i;

    /* Check the number of arguments */
    if(argc < 3) {
        error("This command requires at least two arguments", 1);
    }

    /* All arguments must be non-empty */
    if(argv[1][0] == '\0') {
        error("The delimiter must be non-empty", 2);
    }

    /* Read the delimiter */
    delimiter = argv[1][0];

    /* By default, every column is ignored */
    for(i = 0; i < MAX_COLUMNS; i++) {
        column_selects[i] = COLUMN_IGNORED;
    }

    /* Read the selected column numbers */
    for(i = 2; i < argc; i++) {
        if(sscanf(argv[i], "%u", &last_column) != 1) {
            error("The column argument is not a unsigned integer", 2);
        }

        if(last_column >= MAX_COLUMNS) {
            error("The column number is out of range", 2);
        }

        column_selects[last_column] = COLUMN_SELECTED;
    }

    /* Find the first column to output */
    for(i = 0; i < MAX_COLUMNS; i++) {
        if(column_selects[i] == COLUMN_SELECTED) {
            first_column = i;
            break;
        }
    }

    automaton();
}
