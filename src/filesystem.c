#include "kernel.h"
#include "std_lib.h"
#include "filesystem.h"

void fsInit() {
    struct map_fs map_fs_buf;
    int i = 0;

    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    for (i = 0; i < 16; i++) map_fs_buf.is_used[i] = true;
    for (i = 256; i < 512; i++) map_fs_buf.is_used[i] = true;
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
}

// Implementasi fsRead
void fsRead(struct file_metadata* metadata, enum fs_return* status) {
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int i, j;
    bool found = false;

    // Membaca filesystem dari disk ke memory
    readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector(((byte*)&node_fs_buf) + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);
    readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Iterasi setiap item node untuk mencari node yang sesuai
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == metadata->parent_index &&
            strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name)) {
            found = true;
            break;
        }
    }

    // Jika node tidak ditemukan
    if (!found) {
        *status = FS_R_NODE_NOT_FOUND;
        return;
    }

    // Jika node adalah direktori
    if (node_fs_buf.nodes[i].data_index == FS_NODE_D_DIR) {
        *status = FS_R_TYPE_IS_DIRECTORY;
        return;
    }

    // Jika node adalah file
    metadata->filesize = 0;
    for (j = 0; j < FS_MAX_SECTOR; j++) {
        if (data_fs_buf.datas[node_fs_buf.nodes[i].data_index].sectors[j] == 0x00) {
            break;
        }
        readSector(metadata->buffer + j * SECTOR_SIZE, data_fs_buf.datas[node_fs_buf.nodes[i].data_index].sectors[j]);
        metadata->filesize += SECTOR_SIZE;
    }

    // Set status dengan FS_SUCCESS
    *status = FS_SUCCESS;
}

// Implementasi fsWrite
void fsWrite(struct file_metadata* metadata, enum fs_return* status) {
    struct map_fs map_fs_buf;
    struct node_fs node_fs_buf;
    struct data_fs data_fs_buf;
    int i, j, free_node_index = -1, free_data_index = -1, free_blocks = 0;
    bool node_found = false;

    // Membaca filesystem dari disk ke memory
    readSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    readSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    readSector(((byte*)&node_fs_buf) + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);
    readSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Iterasi setiap item node untuk mencari node yang sama
    for (i = 0; i < FS_MAX_NODE; i++) {
        if (node_fs_buf.nodes[i].parent_index == metadata->parent_index &&
            strcmp(node_fs_buf.nodes[i].node_name, metadata->node_name)) {
            *status = FS_W_NODE_ALREADY_EXISTS;
            return;
        }
        if (!node_found && node_fs_buf.nodes[i].node_name[0] == '\0') {
            free_node_index = i;
            node_found = true;
        }
    }

    // Jika node yang kosong tidak ditemukan
    if (free_node_index == -1) {
        *status = FS_W_NO_FREE_NODE;
        return;
    }

    // Iterasi setiap item data untuk mencari data yang kosong
    for (i = 0; i < FS_MAX_DATA; i++) {
        if (data_fs_buf.datas[i].sectors[0] == 0x00) {
            free_data_index = i;
            break;
        }
    }

    // Jika data yang kosong tidak ditemukan
    if (free_data_index == -1) {
        *status = FS_W_NO_FREE_DATA;
        return;
    }

    // Iterasi setiap item map dan hitung blok yang kosong
    for (i = 0; i < SECTOR_SIZE; i++) {
        if (!map_fs_buf.is_used[i]) {
            free_blocks++;
        }
    }

    // Jika blok yang kosong kurang dari yang dibutuhkan
    if (free_blocks < (metadata->filesize / SECTOR_SIZE + (metadata->filesize % SECTOR_SIZE != 0))) {
        *status = FS_W_NOT_ENOUGH_SPACE;
        return;
    }

    // Set nama dari node, parent index, dan data index
    strcpy(node_fs_buf.nodes[free_node_index].node_name, metadata->node_name);
    node_fs_buf.nodes[free_node_index].parent_index = metadata->parent_index;
    node_fs_buf.nodes[free_node_index].data_index = free_data_index;

    // Penulisan data
    j = 0;
    for (i = 0; i < SECTOR_SIZE && j < (metadata->filesize / SECTOR_SIZE + (metadata->filesize % SECTOR_SIZE != 0)); i++) {
        if (!map_fs_buf.is_used[i]) {
            data_fs_buf.datas[free_data_index].sectors[j] = i;
            writeSector(metadata->buffer + j * SECTOR_SIZE, i);
            map_fs_buf.is_used[i] = true;
            j++;
        }
    }

    // Tulis kembali filesystem yang telah diubah ke dalam disk
    writeSector(&map_fs_buf, FS_MAP_SECTOR_NUMBER);
    writeSector(&node_fs_buf, FS_NODE_SECTOR_NUMBER);
    writeSector(((byte*)&node_fs_buf) + SECTOR_SIZE, FS_NODE_SECTOR_NUMBER + 1);
    writeSector(&data_fs_buf, FS_DATA_SECTOR_NUMBER);

    // Set status dengan FS_SUCCESS
    *status = FS_SUCCESS;
}
