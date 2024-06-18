#include "std_lib.h"

int div(int a, int b) {
  int res = 0;
  while (a >= b) {
    a -= b;
    res++;
  }
  return res;
}

int mod(int a, int b) {
  while (a >= b) a -= b;
  return a;
}

void memcpy(byte* dst, byte* src, unsigned int size) {
  unsigned int i;
  for (i = 0; i < size; i++) dst[i] = src[i];
}

unsigned int strlen(char* str) {
  unsigned int len = 0;
  while (str[len] != '\0') len++;
  return len;
}

bool strcmp(char* str1, char* str2) {
  unsigned int i = 0;
  while (str1[i] != '\0' && str2[i] != '\0') {
    if (str1[i] != str2[i]) return false;
    i++;
  }
  return str1[i] == str2[i];
}

void strcpy(char* dst, char* src) {
  unsigned int i = 0;
  while (src[i] != '\0') {
    dst[i] = src[i];
    i++;
  }
  dst[i] = '\0';
}

void clear(byte* buf, unsigned int size) {
  unsigned int i;
  for (i = 0; i < size; i++) buf[i] = 0;
}

void itoa(int num, char* str) {
    int i = 0, j, temp;
    bool is_negative = false;

    if (num == 0) {
        str[i++] = '0';
        str[i] = '\0';
        return;
    }

    if (num < 0) {
        is_negative = true;
        num = -num;
    }

    while (num != 0) {
        str[i++] = (char)mod(num, 10) + '0';  // Menggunakan fungsi mod dari std_lib.h
        num = div(num, 10);                   // Menggunakan fungsi div dari std_lib.h
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = '\0';

    // Reverse the string
    for (j = 0, i--; j < i; j++, i--) {
        temp = str[j];
        str[j] = str[i];
        str[i] = temp;
    }
}
