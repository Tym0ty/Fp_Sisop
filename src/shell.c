#include "shell.h"
#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void shell() {
  char buf[64];
  char cmd[64];
  char arg[2][64];

  byte cwd = FS_NODE_P_ROOT;

  while (true) {
    printString("MengOS:");
    printCWD(cwd);
    printString("$ ");
    readString(buf);
    parseCommand(buf, cmd, arg);

    if (strcmp(cmd, "cd")) cd(&cwd, arg[0]);
    else if (strcmp(cmd, "ls")) ls(cwd, arg[0]);
    else if (strcmp(cmd, "mv")) mv(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cp")) cp(cwd, arg[0], arg[1]);
    else if (strcmp(cmd, "cat")) cat(cwd, arg[0]);
    else if (strcmp(cmd, "mkdir")) mkdir(cwd, arg[0]);
    else if (strcmp(cmd, "clear")) clearScreen();
    else printString("Invalid command\n");
  }
}

// TODO: 4. Implement printCWD function
void printCWD(byte cwd) {
    struct node_fs node_fs_buf;
    char path[FS_MAX_NODE][MAX_FILENAME];
    int path_index = 0;
    int i;

    // If cwd is 0xFF, print root and return
    if (cwd == 0xFF) {
        printString("/");
        return;
    }

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

    // Traverse the path from cwd to root
    while (cwd != FS_NODE_P_ROOT) {
        strcpy(path[path_index], node_fs_buf.nodes[cwd].node_name);
        cwd = node_fs_buf.nodes[cwd].parent_index;
        path_index++;
    }

    // Print the path from root to cwd
    for (i = path_index - 1; i >= 0; i--) {
        printString("/");
        printString(path[i]);
    }

    // Add a final slash to indicate directory
    printString("/");
}

// TODO: 5. Implement parseCommand function
void parseCommand(char* buf, char* cmd, char arg[2][64]) {
    int i = 0, j = 0, arg_index = 0;

    // Initialize cmd and arg
    clear((byte*)cmd, 64);
    clear((byte*)arg, 2 * 64);

    // Skip leading spaces
    while (buf[i] == ' ') i++;

    // Parse the command
    while (buf[i] != ' ' && buf[i] != '\0') {
        cmd[j++] = buf[i++];
    }
    cmd[j] = '\0';

    // Skip spaces before arguments
    while (buf[i] == ' ') i++;

    // Parse the arguments
    while (buf[i] != '\0' && arg_index < 2) {
        j = 0;
        while (buf[i] != ' ' && buf[i] != '\0') {
            arg[arg_index][j++] = buf[i++];
        }
        arg[arg_index][j] = '\0';
        arg_index++;
        
        // Skip spaces before the next argument
        while (buf[i] == ' ') i++;
    }
}

// TODO: 6. Implement cd function
void cd(byte* cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    char current_dir_str[4];  // Buffer untuk menyimpan hasil konversi integer ke string

    // Read the filesystem nodes into memory
    readSector((byte *)&node_fs_buf, FS_NODE_SECTOR_NUMBER);

    // Debug: print current directory index
    printString("Debug: Current directory index: ");
    itoa(*cwd, current_dir_str);
    printString(current_dir_str);
    printString("\n");

    // Debug: print all nodes
    printString("Debug: Node list:\n");
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] != '\0') {
            printString("Index ");
            itoa(i, current_dir_str);
            printString(current_dir_str);
            printString(": ");
            printString(node_fs_buf.nodes[i].node_name);
            printString(" (Parent: ");
            itoa(node_fs_buf.nodes[i].parent_index, current_dir_str);
            printString(current_dir_str);
            printString(")\n");
        }
    }

    // Handle special cases for ".." and "/"
    if (strcmp(dirname, "..") == 0) {
        if (*cwd != FS_NODE_P_ROOT) {
            *cwd = node_fs_buf.nodes[*cwd].parent_index;
            printString("Debug: Changed directory to parent\n");
        } else {
            printString("Debug: Already at root, cannot go to parent\n");
        }
        return;
    } else if (strcmp(dirname, "/") == 0) {
        *cwd = FS_NODE_P_ROOT;
        printString("Debug: Changed directory to root\n");
        return;
    }

    // Search for the directory in the current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == *cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                *cwd = i;
                printString("Debug: Changed directory to ");
                printString(dirname);
                printString(" (index: ");
                itoa(i, current_dir_str);
                printString(current_dir_str);
                printString(")\n");
                return;
            } else {
                printString("cd: not a directory\n");
                return;
            }
        }
    }

    // If the directory is not found
    printString("cd: no such directory\n");
}


// TODO: 7. Implement ls function
void ls(byte cwd, char* dirname) {
    struct node_fs node_fs_buf;
    int i;
    bool is_current_dir = false;

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

    // If dirname is "." or empty, list the contents of the current directory
    if (strcmp(dirname, ".") || strlen(dirname) == 0) {
        is_current_dir = true;
    }

    // If it's the current directory, list the contents of cwd
    if (is_current_dir) {
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd) {
                printString(node_fs_buf.nodes[i].node_name);
                if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                    printString("/\n");
                } else {
                    printString("\n");
                }
            }
        }
        return;
    }

    // Search for the directory in the current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, dirname)) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                // List the contents of the found directory
                byte dir_index = i;
                for (i = 0; i < FS_MAX_NODE; i++) {
                    if (node_fs_buf.nodes[i].parent_index == dir_index) {
                        printString(node_fs_buf.nodes[i].node_name);
                        if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                            printString("/\n");
                        } else {
                            printString("\n");
                        }
                    }
                }
                return;
            } else {
                printString("ls: not a directory\n");
                return;
            }
        }
    }

    // If the directory is not found
    printString("ls: no such directory\n");
}

// TODO: 8. Implement mv function
void mv(byte cwd, char* src, char* dst) {}

// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {}

// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    int i, free_node_index = -1;

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);
    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);

    // Check if directory with the same name already exists
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd && strcmp(node_fs_buf.nodes[i].node_name, dirname) == 0) {
            printString("Directory already exists\n");
            return;
        }
        if (free_node_index == -1 && node_fs_buf.nodes[i].node_name[0] == '\0') {
            free_node_index = i;
        }
    }

    // If no free node found, print error and return
    if (free_node_index == -1) {
        printString("No free node available\n");
        return;
    }

    // Create new directory entry in the free node
    strcpy(node_fs_buf.nodes[free_node_index].node_name, dirname);
    node_fs_buf.nodes[free_node_index].parent_index = cwd;
    node_fs_buf.nodes[free_node_index].data_index = FS_NODE_D_DIR;

    // Write back the updated nodes to the filesystem
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER);

    printString("Directory created\n");
}
