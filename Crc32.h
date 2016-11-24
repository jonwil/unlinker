#pragma once

class StringClass;
unsigned long CRC_MS(const unsigned char* data, unsigned long length, unsigned long crc = 0);
unsigned long CRC_Memory(const unsigned char* data, unsigned long length, unsigned long crc = 0);
unsigned long CRC_String(const char *data,unsigned long crc);
unsigned long CRC_Stringi(char  const*, unsigned long = 0);
