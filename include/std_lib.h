#ifndef STD_LIB_H
#define STD_LIB_H

#include "std_type.h"

int div(int a, int b);
int mod(int a, int b);
void memcpy(byte* dst, byte* src, unsigned int size);
unsigned int strlen(char* str);
bool strcmp(char* str1, char* str2);
void strcpy(char* dst, char* src);
void clear(byte* buf, unsigned int size);
void itoa(int num, char* str);  // Tambahkan deklarasi fungsi itoa

#endif
