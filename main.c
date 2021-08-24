#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

// Macros
#define STOP_COMMAND "STOP\n"
#define STOP_CHAR '\3'
#define STOP_STR "\3"
#define LINE_SEPARATOR "\n"
#define LINE_COUNT 50
#define LINE_LENGTH 1000
#define BUFFER_COUNT 4

// Buffer Structure
struct buffer 
{
    char buffer[LINE_COUNT][LINE_LENGTH];
    pthread_mutex_t mutex;
    pthread_cond_t is_full;
};

// Global Pipeline Buffers
struct buffer *buffer_1, *buffer_2, *buffer_3, *buffer_4;
struct buffer* buffers[BUFFER_COUNT];

void initialize_buffers(void) 
{
    // Initialize Buffers Array
    buffer_1 = calloc(1, sizeof(struct buffer));
    buffer_2 = calloc(1, sizeof(struct buffer));
    buffer_3 = calloc(1, sizeof(struct buffer));
    buffer_4 = calloc(1, sizeof(struct buffer));
    buffers[0] = buffer_1;
    buffers[1] = buffer_2;
    buffers[2] = buffer_3;
    buffers[3] = buffer_4;

    // Intializes Buffers
    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        memset(buffers[i]->buffer, 0, sizeof(char) * LINE_COUNT * LINE_LENGTH);
        buffers[i]->mutex = (pthread_mutex_t)PTHREAD_MUTEX_INITIALIZER;
        buffers[i]->is_full = (pthread_cond_t)PTHREAD_COND_INITIALIZER;
    }
}

void deallocate_buffers(void)
{
    for (int i = 0; i < BUFFER_COUNT; i++)
    {
        free(buffers[i]);
    }
}

// Buffer Functionality
void write_to_buffer_line(struct buffer* buffer, int line, char input[])
{
    // Lock Mutex before writing character to the buffer
    pthread_mutex_lock(&buffer->mutex);

    // write input to buffer line
    strcpy(buffer->buffer[line], input);

    // Send signal to consumer that buffer is not empty
    pthread_cond_signal(&buffer->is_full);    

    // Unlock Mutex
    pthread_mutex_unlock(&buffer->mutex);
}

void read_from_buffer_line(struct buffer* buffer, int line, char output[])
{
    // Local Variables
    ssize_t line_length = strlen(buffer->buffer[line]);

    // Lock Mutex before writing character to the buffer
    pthread_mutex_lock(&buffer->mutex);

    // Wait if the buffer is empty
    while (line_length == 0)
    {
        pthread_cond_wait(&buffer->is_full, &buffer->mutex);
        line_length = strlen(buffer->buffer[line]);
    }
    
    // read buffer line to output
    strcpy(output, buffer->buffer[line]);

    // Unlock Mutex
    pthread_mutex_unlock(&buffer->mutex);
}

// Helper Functions
// - Replace subtring
// - - Followed tutorial from thread: https://stackoverflow.com/questions/32413667/replace-all-occurrences-of-a-substring-in-a-string-in-c
void replace_substring(char* string, const char* substring, char* replacement)
{
    // Function Constants
    static const int REPLACING = 1;

    // Local Variables
    char buffer[LINE_LENGTH] = { 0 };
    char *pointer = &buffer[0];
    const char *tempString = string;
    size_t lenSubstring = strlen(substring);
    size_t lenReplacement = strlen(replacement);

    // Subtring Replacement Functionality
    while (REPLACING) {
        const char *substringIndex = strstr(tempString, substring);

        // Check if another substring exists in the string
        if (substringIndex == NULL) {
            // No more substrings to replace, exit loop
            strcpy(pointer, tempString);
            break;
        }

        // Copy string up to index of found substring
        memcpy(pointer, tempString, substringIndex - tempString);
        pointer += substringIndex - tempString;

        // Copy string after index of found substring
        memcpy(pointer, replacement, lenReplacement);
        pointer += lenReplacement;

        // Increment the pointer to continue searching at the index after the replaced substring
        tempString = substringIndex + lenSubstring;
    }

    // Copy the string with the replaced substrings to the original string
    strcpy(string, buffer);
}

int input_contains_stop(char* input)
{
    char* position_pointer = strstr(input, STOP_COMMAND);
    // Stop command not in string
    if (position_pointer == NULL)
    {
        return -1;
    }
    // Get index of stop command
    int position = position_pointer - input;
    // Stop command is first in string
    if (position == 0 && !isalnum(input[6]) && !ispunct(input[6]))
    {
        return position;
    }
    // Stop command is not first in string
    if (!isalnum(input[position-1]) && !ispunct(input[position-1]))
    {
        return position;
    }
    // Stop command surrounded by negating character(s)
    return -1;
}

// Input Thread
// - Consumes from stdin
// - Produces to Buffer 1
void* process_input(void* args) {    
    // Local Variables
    bool running = true;
    bool stopping = false;
    char input[1000] = {0};
    int buffer_line = 0;
    int position = -1;

    // Thread Task
    while(running && buffer_line != 50)
    { 
        // Get line from stdin
        fgets(input, LINE_LENGTH, stdin);

        // Parse for stop command
        position = input_contains_stop(input);
        if (position != -1)
        {
            input[position] = STOP_CHAR;
            stopping = true;
        }

        // Output to production buffer
        write_to_buffer_line(buffer_1, buffer_line, input); 

        // exit if the line is the stop command
        if (stopping)
        {
            running = false;
        }

        // Move on to the next line of input
        buffer_line += 1;    
    }
    // Thread Task Complete
    return NULL;
}

// Line Separator Thread
// - Consumes from Buffer 1
// - Produces to Buffer 2
void* separate_lines(void* args) {
    // Local Variables
    bool running = true;
    bool stopping = false;
    int buffer_line = 0;
    char input[1000] = {0};

    // Thread Task
    while(running)
    {
        // Get input from consumer buffer
        read_from_buffer_line(buffer_1, buffer_line, input);

        // Check for stop marker
        if (strstr(input, STOP_STR) != NULL)
        {
            stopping = true;
        }

        // Separator Substitution
        replace_substring(input, "\n", " ");
   
        // Output to production buffer
        write_to_buffer_line(buffer_2, buffer_line, input);        
  
        // exit if the consume buffer is the stop command
        if (stopping)
        {
            running = false;
        }

        // Move on to the next line of input
        buffer_line += 1;
    }
    // Thread Task Complete
    return NULL;
}

// Plus Sign Thread
// - Consumes from Buffer 2
// - Produces to Buffer 3
void* replace_plus_signs(void* args) {
    // Local Variables
    bool running = true;
    bool stopping = false;
    int buffer_line = 0;
    char input[1000] = {0};

    // Thread Task
    while (running)
    {
        // Get input from consumer buffer
        read_from_buffer_line(buffer_2, buffer_line, input);

        // Check for stop marker
        if (strstr(input, STOP_STR) != NULL)
        {
            stopping = true;
        }

        // Double-plus Substitution
        replace_substring(input, "++", "^");

        // Output to the production buffer
        write_to_buffer_line(buffer_3, buffer_line, input);

        // exit if the consume buffer is the stop command
        if (stopping)
        {
            running = false;
        }

        // Move on to the next line of input
        buffer_line += 1;
    }
    // Thread Task Complete
    return NULL;
}

// Output Thread
// - Consumes from Buffer 3
// - Produces to stdout
void* process_output(void* args) {
    // Local Variables
    bool running = true;
    bool stopping = false;
    int buffer_line = 0;
    char input[1000] = {0};
    char throughput[LINE_COUNT * LINE_LENGTH] = {0};
    char output[81] = {0};
    int remaining_chars = 80;
    int i = 0; // output array index
    int j = 0; // throughput array index

    // Thread Task
    while (running)
    {

        // Get input from consumer buffer
        read_from_buffer_line(buffer_3, buffer_line, input);

        // Add input to the total input received
        strcat(throughput, input);

        // Check for stop marker
        if (strstr(input, STOP_STR) != NULL)
        {
            stopping = true;
        }

        // Print 80 characters at a time from total input received
        while (j < (strlen(throughput)) && throughput[j] != STOP_CHAR)
        {
            // Copy character from total input to output
            output[i] = throughput[j];

            // Adjust positioning to account for newly added character
            i += 1;
            j += 1;
            remaining_chars -= 1;
            
            // 80 characters reached, output to stdout
            if (remaining_chars == 0)
            {
                printf("%s\n",output);
                fflush(stdout);

                // reset output array
                memset(output, 0, 81);
                i = 0;
                remaining_chars = 80;
            }
        }

        // exit if the consume buffer is the stop command
        if (stopping)
        {
            running = false;
        }

        // Move on to the next line of input
        buffer_line += 1;
    }
    // Thread Task Complete
    return NULL;
}

// Main Program - Thread Creator
int main(void) {
    // Set up global pipeline buffers
    initialize_buffers();
    
    // Create variables for thread IDs
    pthread_t thread_1, thread_2, thread_3, thread_4;

    // Create Pipeline Threads
    pthread_create(&thread_1, NULL, process_input, NULL);
    pthread_create(&thread_2, NULL, separate_lines, NULL);
    pthread_create(&thread_3, NULL, replace_plus_signs, NULL);
    pthread_create(&thread_4, NULL, process_output, NULL);

    // Wait for threads to finish executing
    pthread_join(thread_1, NULL);
    pthread_join(thread_2, NULL);
    pthread_join(thread_3, NULL);
    pthread_join(thread_4, NULL);

    // Deallocate memory used for global pipeline buffers
    deallocate_buffers();

    return EXIT_SUCCESS;
}