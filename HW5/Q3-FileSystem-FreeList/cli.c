#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT 512
#define MAX_TOKENS 8

// simple tokenizer
int tokenize(char *line, char *tokens[], int max_tokens) {
    int count = 0;
    char *p = strtok(line, " \t\n");
    while (p && count < max_tokens) {
        tokens[count++] = p;
        p = strtok(NULL, " \t\n");
    }
    return count;
}

void cmd_help() {
    printf("Available commands:\n");
    printf("  init <path>\n");
    printf("  open <filename> <flags>\n    flags:\n      0 -> open existing file\n      1 -> create & open new file\n");
    printf("  read <pos> <nbytes>\n");
    printf("  write <pos> <nbytes>\n");
    printf("  shrink <newsize>\n");
    printf("  file_stats\n");
    printf("  fs_stats\n");
    printf("  rm\n");
    printf("  close\n");
    printf("  viz\n    show free spaces\n");
    printf("  help\n");
    printf("  exit\n");
}

int main() {
    char line[MAX_INPUT];

    printf("FS CLI — type 'help' for commands.\n");

    while (1) {
        printf("myfs> ");
        fflush(stdout);

        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }

        // tokenize input
        char *tokens[MAX_TOKENS];
        int tokc = tokenize(line, tokens, MAX_TOKENS);
        if (tokc == 0) continue;

        // --- Commands ---
        if (strcmp(tokens[0], "help") == 0) {
            cmd_help();
        }

        else if (strcmp(tokens[0], "exit") == 0) {
            printf("Bye!\n");
            break;
        }

        else if (strcmp(tokens[0], "init") == 0) {
            if (tokc != 2) {
                printf("Usage: init <path>\n");
                continue;
            }
            if (!init_fs(tokens[1])) {
                printf("Failed to initialize filesystem.\n");
            }
        }

        else if (strcmp(tokens[0], "open") == 0) {
            if (tokc != 3) {
                printf("Usage: open <filename> <flags>\n    flags:\n      0 -> open existing file\n      1 -> create & open new file\n\n");
                continue;
            }

            const char *filename = tokens[1];
            uint32_t flags = atoi(tokens[2]); // ASCII to integer

            file_entry_t ent;
            uint32_t off = find_file(filename, &ent);

            if (off == 0) { // file does not yet exist
                if (flags & 1) {  // let's say CREATE flag = 1
                    off = create_file(filename);
                    if (off == 0) continue;

                    load_file_entry(off, &ent);
                    printf("File created and opened: %s\n", ent.name);
                } else {
                    printf("Error: file not found.\n");
                    continue;
                }
            } else if (flags) {
                printf("A file already exists with name: %s\n", ent.name);
            } else {
                printf("File opened: %s (size = %u bytes)\n", ent.name, ent.size);
            }

            g_open_file_offset = off;
        }

        else if (strcmp(tokens[0], "write") == 0) {
            if (tokc != 3) {
                printf("Usage: write <pos> <nbytes>\n");
                continue;
            }
            uint32_t pos = atoi(tokens[1]);
            uint32_t nbytes = atoi(tokens[2]);
            write_file(pos, nbytes);
        }

        else if (strcmp(tokens[0], "read") == 0) {
            if (tokc != 3) {
                printf("Usage: read <pos> <nbytes>\n");
                continue;
            }
            uint32_t pos = atoi(tokens[1]);
            uint32_t nbytes = atoi(tokens[2]);
            read_file(pos, nbytes);
        }

        else if (strcmp(tokens[0], "shrink") == 0) {
            if (tokc != 2) { printf("Usage: shrink <newsize>\n"); continue; }
            shrink_file(atoi(tokens[1]));
        }

        else if (strcmp(tokens[0], "rm") == 0) {
            remove_file();
        }

        else if (strcmp(tokens[0], "file_stats") == 0) {
            if (tokc != 1) {
                printf("Usage: file_stats\n");
                continue;
            }
            file_stats();
        }

        else if (strcmp(tokens[0], "fs_stats") == 0) {
            if (tokc != 1) {
                printf("Usage: fs_stats\n");
                continue;
            }
            fs_stats();
        }

        else if (strcmp(tokens[0], "viz") == 0) {
            if (tokc != 1) {
                printf("Usage: viz\n");
                continue;
            }
            visualize_freelist();
        }


        else if (strcmp(tokens[0], "close") == 0) {
            if (tokc != 1) {
                printf("Usage: close\n");
                continue;
            }
            close_file();
        }

        else {
            printf("Unknown command: %s\n", tokens[0]);
            printf("Type 'help' for command list.\n");
        }
    }

    return 0;
}
