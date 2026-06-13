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
    printf("\nAvailable commands:\n");

    printf("\nFilesystem:\n");
    printf("  init <path>                    initialize or open filesystem\n");
    printf("  fs_stats                       show filesystem statistics\n");
    printf("  viz                            visualize free space\n");

    printf("\nFiles:\n");
    printf("  open <filename> <flags>        open or create file\n");
    printf("                                 flags:\n");
    printf("                                   0 -> open existing file\n");
    printf("                                   1 -> create & open new file\n");
    printf("  close                          close currently open file\n");
    printf("  file_stats                     show opened file metadata\n");
    printf("  read <pos> <nbytes>            read bytes from file\n");
    printf("  write <pos> <nbytes>           write bytes to file (stdin)\n");
    printf("  shrink <newsize>               truncate file\n");
    printf("  rm                             remove currently open file\n");

    printf("\nUsers & Groups:\n");
    printf("  login <username>               login as user\n");
    printf("  users                          list all users and groups\n");
    printf("  useradd <username>             add new user (root only)\n");
    printf("  userdel <username>             delete user (root only)\n");
    printf("  groupadd <groupname>           add new group (root only)\n");
    printf("  groupdel <groupname>           delete group (root only)\n");
    printf("  usermod -aG <group> <user>     add user to group (root only)\n");

    printf("\nPermissions:\n");
    printf("  chmod <mode>                   change file permissions (e.g. 644)\n");
    printf("  chown <user>:<group>           change file owner and group (root only)\n");
    printf("  chgrp <group>                  change file group\n");
    printf("  getfacl                        show file ACL\n");
    
    printf("\nStressTest:\n");
    printf("  stress                         test filesystem performance under stress\n");

    printf("\nMisc:\n");
    printf("  help                           show this help message\n");
    printf("  exit                           exit CLI\n\n");
}


int main() {
    char line[MAX_INPUT];

    printf("FS CLI - type 'help' for commands.\n");

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
                    if (off == 0) {
                        printf("Failed to create file!\n");
                        continue;
                    }
                    load_file_entry(off, &ent);
                    printf("File created and opened: %s\n", ent.name);
                } else {
                    printf("Error: file not found.\n");
                    continue;
                }
            } else if (flags) {
                printf("A file already exists with name: %s\n", ent.name);
                continue;
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
        
        // -------------------------- HW6 --------------------------
        // -------------------------- Users & Permissions --------------------------
        else if (strcmp(tokens[0], "useradd") == 0) {
            user_add(tokens[1]);
        }

        else if (strcmp(tokens[0], "login") == 0) {
            if (tokc != 2) {
                printf("Usage: login <username>\n");
                continue;
            }
            if (login_user(tokens[1])) {
                printf("Logged in as %s (UID: %u)\n", tokens[1], g_current_user_id);
            } else {
                printf("User not found.\n");
            }
        }

        else if (strcmp(tokens[0], "userdel") == 0) {
            if (tokc != 2) {
                printf("Usage: userdel <username>\n");
                continue;
            }
            user_del(tokens[1]);
        }

        else if (strcmp(tokens[0], "groupadd") == 0) {
            if (tokc != 2) {
                printf("Usage: groupadd <groupname>\n");
                continue;
            }
            group_add(tokens[1]);
        }

        else if (strcmp(tokens[0], "groupdel") == 0) {
            if (tokc != 2) {
                printf("Usage: groupdel <groupname>\n");
                continue;
            }
            group_del(tokens[1]);
        }

        else if (strcmp(tokens[0], "usermod") == 0) {
            if (tokc == 4 && strcmp(tokens[1], "-aG") == 0) {
                usermod_ag(tokens[2], tokens[3]);
            }
            else
                printf("Usage: - usermod -aG <group> <username>\n");
        }

        else if (strcmp(tokens[0], "chmod") == 0) {
            // Check if a file is currently open
            if (g_open_file_offset == 0) {
                printf("Error: No file is open. Use 'open' first.\n");
                continue;
            }

            // Check for the correct number of arguments
            if (tokc != 2) {
                printf("Usage: chmod <mode> (e.g., 644 or 755)\n");
                continue;
            }

            // Convert and validate the mode
            uint32_t mode = (uint32_t)atoi(tokens[1]);

            // Basic validation: Unix permissions usually don't exceed 777
            if (mode > 777) {
                printf("Error: Invalid mode. Use standard 3-digit permissions (000-777).\n");
                continue;
            }

            chmod_file(mode);
        }

        else if (strcmp(tokens[0], "chown") == 0) {
            if (tokc != 2) { printf("Usage: chown <user>:<group>\n"); continue; }
            char *u = strtok(tokens[1], ":");
            char *g = strtok(NULL, ":");
            if (u && g) chown_file(u, g);
        }

        else if (strcmp(tokens[0], "chgrp") == 0) {
            if (tokc != 2) { printf("Usage: chgrp <group>\n"); continue; }
            chgrp_file(tokens[1]);
        }

        else if (strcmp(tokens[0], "getfacl") == 0) {
            getfacl_file();
        }
        
        else if (strcmp(tokens[0], "users") == 0) {
            if (tokc != 1) {
                printf("Usage: users\n");
                continue;
            }
            list_users_and_groups();
        }
        // -------------------------- HW7 --------------------------
        // ---------------------- StressTest ----------------------
        else if (strcmp(tokens[0], "stress") == 0) {
            if (tokc != 1) {
                printf("Usage: stress\n");
                continue;
            }
            stress_test();
        }
        else {
            printf("Unknown command: %s\n", tokens[0]);
            printf("Type 'help' for command list.\n");
        }
    }

    return 0;
}
