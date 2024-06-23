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
    char current_dir_str[4]; 

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Handle special cases for ".." and "/"
    if (strcmp(dirname, "..") == 1) {
        if (*cwd != FS_NODE_P_ROOT) {
            *cwd = node_fs_buf.nodes[*cwd].parent_index;
        } else {
            printString("Debug: Already at root, cannot go to parent\n");
        }
        return;
    } else if (strcmp(dirname, "/") == 1) {
        *cwd = FS_NODE_P_ROOT;
        return;
    }

    // Search for the directory in the current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == *cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, dirname) == 1) {
            if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
                *cwd = i;
                printString("Changed directory to ");
                printString(dirname);
                printString("\n");
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
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

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
void mv(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    int i;
    int src_index = -1;
    byte dst_dir_index = cwd;
    int dst_len = strlen(dst);
    int slash_index = -1;
    char target_dir[MAX_FILENAME];
    char new_filename[MAX_FILENAME];

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Find the source file index in the current working directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, src)) {
            src_index = i;
            break;
        }
    }

    if (src_index == -1) {
        printString("mv: file not found\n");
        return;
    }

    // Find the last '/' to split target directory and new filename
    for (i = dst_len - 1; i >= 0; i--) {
        if (dst[i] == '/') {
            slash_index = i;
            break;
        }
    }

    // Find the last '/' to split target directory and new filename
    for (i = dst_len - 1; i >= 0; i--) {
        if (dst[i] == '/') {
            slash_index = i;
            break;
        }
    }


    // Copy target directory
    if (slash_index > 0) {
        // Extract target directory
        for (i = 0; i < slash_index; i++) {
            target_dir[i] = dst[i];
        }
        target_dir[slash_index] = '\0';

        // Extract new filename
        for (i = slash_index + 1; i < dst_len; i++) {
            new_filename[i - slash_index - 1] = dst[i];
        }
        new_filename[dst_len - slash_index - 1] = '\0';
    } else {
        // No '/' found, new filename is the entire dst
        strcpy(target_dir, dst);
        strcpy(new_filename, src);  // Use the source filename as new filename
    }

    if (target_dir[0] == '/' && (target_dir[1] == '\0' || target_dir[1] == '/')) {
        // Move to root directory
        dst_dir_index = FS_NODE_P_ROOT;
    }
    // Check if target_dir starts with ".."
    else if (target_dir[0] == '.' && target_dir[1] == '.' && (target_dir[2] == '\0' || target_dir[2] == '/')) {
        // Move to parent directory
        dst_dir_index = node_fs_buf.nodes[cwd].parent_index;
    } else {
        // Move to specified directory
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd &&
                strcmp(node_fs_buf.nodes[i].node_name, target_dir)) {
                dst_dir_index = i;
                break;
            }
        }
    }


    // Update the parent_index of the source file
    node_fs_buf.nodes[src_index].parent_index = dst_dir_index;
    strcpy(node_fs_buf.nodes[src_index].node_name, new_filename);

    // Write the updated nodes back to the filesystem
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    printString("File moved successfully\n");
}


// TODO: 9. Implement cp function
void cp(byte cwd, char* src, char* dst) {
    struct node_fs node_fs_buf;
    struct file_metadata src_metadata;
    struct file_metadata dst_metadata;
    enum fs_return status;
    int i, dst_index = -1;
    byte target_dir_index = cwd;
    char target_dir[64];
    char new_filename[64];
    int dst_len = strlen(dst);
    int slash_index = -1;

    // Initialize source metadata
    src_metadata.parent_index = cwd;
    strcpy(src_metadata.node_name, src);

    // Read the source file
    fsRead(&src_metadata, &status);

    if (status != FS_SUCCESS) {
        printString("cp: source file not found\n");
        return;
    }

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    // Find the last '/' to split target directory and new filename
    for (i = dst_len - 1; i >= 0; i--) {
        if (dst[i] == '/') {
            slash_index = i;
            break;
        }
    }

    // Copy target directory and filename
    if (slash_index > 0) {
        // Extract target directory
        for (i = 0; i < slash_index; i++) {
            target_dir[i] = dst[i];
        }
        target_dir[slash_index] = '\0';

        // Extract new filename
        for (i = slash_index + 1; i < dst_len; i++) {
            new_filename[i - slash_index - 1] = dst[i];
        }
        new_filename[dst_len - slash_index - 1] = '\0';

        // Search for the target directory
        for (i = 0; i < FS_MAX_NODE; i++) {
            if (node_fs_buf.nodes[i].parent_index == cwd &&
                strcmp(node_fs_buf.nodes[i].node_name, target_dir) == 1) {
                target_dir_index = i;
                break;
            }
        }

        // If target directory not found, return error
        if (i == FS_MAX_NODE) {
            printString("cp: target directory not found\n");
            return;
        }
    } else {
        strcpy(new_filename, dst);
    }

    // Handle special cases for "/" (root directory) and "../" (parent directory)
    if (slash_index == 0 && dst[0] == '/') {
        target_dir_index = FS_NODE_P_ROOT;
    } else if (slash_index == 1 && dst[0] == '.' && dst[1] == '.') {
        target_dir_index = node_fs_buf.nodes[cwd].parent_index;
    }

    // Ensure the destination filename does not already exist in the target directory
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == target_dir_index &&
            strcmp(node_fs_buf.nodes[i].node_name, new_filename) == 1) {
            printString("cp: destination file already exists\n");
            return;
        }
    }

    // Find a free node for the new file
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].node_name[0] == '\0') {
            dst_index = i;
            break;
        }
    }

    if (dst_index == -1) {
        printString("cp: no space for new file\n");
        return;
    }

    // Initialize destination metadata
    dst_metadata.parent_index = target_dir_index;
    strcpy(dst_metadata.node_name, new_filename);
    dst_metadata.filesize = src_metadata.filesize;
    memcpy(dst_metadata.buffer, src_metadata.buffer, src_metadata.filesize);

    // Write the destination file
    fsWrite(&dst_metadata, &status);

    if (status != FS_SUCCESS) {
        printString("cp: failed to write to destination\n");
        return;
    }

    // Update the filesystem node for the new file
    strcpy(node_fs_buf.nodes[dst_index].node_name, new_filename);
    node_fs_buf.nodes[dst_index].parent_index = target_dir_index;
    node_fs_buf.nodes[dst_index].data_index = 0x00; // Assume 0x00 is for files

    // Write the updated nodes back to the filesystem
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    printString("File copied successfully\n");
}

// TODO: 10. Implement cat function
void cat(byte cwd, char* filename) {
    struct file_metadata metadata;
    enum fs_return status;

    // Prepare file metadata
    metadata.parent_index = cwd;
    strcpy(metadata.node_name, filename);

    // Read file contents using fsRead
    fsRead(&metadata, &status);

    // Handle different statuses from fsRead
    switch (status) {
        case FS_SUCCESS:
            // Print file contents using printString
            printString(metadata.buffer);
            printString("\n"); // Print newline after entire file contents
            break;
        case FS_R_NODE_NOT_FOUND:
            printString("cat: file not found\n");
            break;
        case FS_R_TYPE_IS_DIRECTORY:
            printString("cat: cannot cat a directory\n");
            break;
        default:
            printString("cat: unknown error\n");
            break;
    }
}


// TODO: 11. Implement mkdir function
void mkdir(byte cwd, char* dirname) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    int i, free_node_index = -1;
    char cwd_str[4]; // Buffer to store the string representation of cwd
    char node_index_str[4]; // Buffer to store the string representation of node index

    // Read the filesystem nodes into memory
    readSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    readSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);
    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);


    // Iterate through the filesystem nodes to check for existing directories
    for (i = 0; i < FS_MAX_NODE; i++) {

        // Check if the node belongs to the current working directory and if the names match
        if (node_fs_buf.nodes[i].parent_index == cwd &&
            strcmp(node_fs_buf.nodes[i].node_name, dirname) == 1) {
            printString("Directory already exists\n");
            return;
        }

        // Find the first free node
        if (free_node_index == -1 && node_fs_buf.nodes[i].node_name[0] == '\0') {
            free_node_index = i;
        }
    }

    // Create new directory entry in the free node
    strcpy(node_fs_buf.nodes[free_node_index].node_name, dirname);
    node_fs_buf.nodes[free_node_index].parent_index = cwd;
    node_fs_buf.nodes[free_node_index].data_index = FS_NODE_D_DIR;


    // Write back the updated nodes to the filesystem
    writeSector(&(node_fs_buf.nodes[0]), FS_NODE_SECTOR_NUMBER);
    writeSector(&(node_fs_buf.nodes[32]), FS_NODE_SECTOR_NUMBER + 1);

    printString("Directory created\n");
}
