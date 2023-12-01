#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include <zephyr/sys/printk.h>

//#include "3D_Print_Variables.h"

int checkUARTDataInput(const char *input, const char *check);
int extractLongLongs(const char *input);

void extractTemperatureData(const char *input, int maxNumbers);
void longLongToChar(long long num, char *str);
void floatToChar(float num, int precision);

uint16_t floatToUint16(float floatValue);
uint16_t returnTempData(int val);

uint8_t returnSDPercentage();

long long returnSDData(int val);

char* returnCharBuffer();