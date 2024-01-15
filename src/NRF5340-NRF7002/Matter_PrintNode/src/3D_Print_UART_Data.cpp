#include "3D_Print_UART_Data.h"

char bufferChar[64];

float temperatures[4];
long long value1, value2;

int checkUARTDataInput(const char *input, const char *check) {
	if ((input == NULL || check == NULL)) {
		return -1;
	}

	char *result = strstr(input, check);

	if (result == NULL) {
		return -1;
	} else {
		return 1;
	}
}

void extractTemperatureData(const char *input, int maxNumbers) {
    if (input == NULL || temperatures == NULL || maxNumbers <= 0) {
        // Check for NULL pointers and invalid parameters
        return;
    }

    int numCount = 0; // Counter for the numbers found
    char *endPtr;     // Pointer to detect the end of the number conversion

    // Loop through the input string
    for (const char *ptr = input; *ptr != '\0'; ++ptr) {
        if (numCount >= maxNumbers) {
            break; // Stop if we've reached the maximum number count
        }

        // Skip non-numeric characters and separators
        while (*ptr != '\0' && !isdigit(*ptr) && *ptr != '-' && *ptr != '.' && *ptr != ',') {
            ptr++;
        }

        if (*ptr == '\0') {
            break; // Exit if we reached the end of the string
        }

        // Try to convert a number starting from the current position
        double num = strtod(ptr, &endPtr);

        if (ptr != endPtr) {
            temperatures[numCount] = num;
            numCount++;
        }

        // Move the pointer to the next character after the converted number
        ptr = endPtr;
    }
}

int extractLongLongs(const char *input) {
    int valuesExtracted = 0;
    const char *startPtr = input;

    while (*startPtr) {
        // Skip non-numeric characters
        while (*startPtr && !isdigit(*startPtr)) {
            startPtr++;
        }

        if (*startPtr) {
            // Use strtoll to extract the long long value
            char *endPtr;
            long long extractedValue = strtoll(startPtr, &endPtr, 10);

            if (endPtr > startPtr) {
                if (valuesExtracted == 0) {
                    value1 = extractedValue;
                } else if (valuesExtracted == 1) {
                    value2 = extractedValue;
                }
                valuesExtracted++;

                startPtr = endPtr;
            } else {
                // If no valid number is found, move to the next character
                startPtr++;
            }
        }
    }

    return (valuesExtracted == 2);
}

void longLongToChar(long long num, char *str) {
    if (num < 0) {
        *str++ = '-';
        num = -num;
    }

    char temp[21]; // Assuming a long long can be represented in at most 20 characters plus the null terminator
    int i = 0;

    do {
        temp[i++] = num % 10 + '0';
        num /= 10;
    } while (num != 0);

    while (i > 0) {
        *str++ = temp[--i];
    }
    *str = '\0';
}

uint8_t returnSDPercentage() {
    if (value2 == 0) {
        // Handle division by zero
        printk("Error: Division by zero.\n");
        return 0;
    }

    // Perform the division
    uint8_t result = (int)((float)((float)value1 / (float)value2) * 100);

    return result;
}

void floatToChar(float num, int precision) {
    int wholePart = (int)num;
    float fractionalPart = num - wholePart;
    
    // Convert the whole part to a string
    int wholeLength = 0;
    if (wholePart == 0) {
        bufferChar[wholeLength++] = '0';
    } else {
        while (wholePart > 0) {
            bufferChar[wholeLength++] = '0' + (wholePart % 10);
            wholePart /= 10;
        }
    }
    
    // Reverse the whole part string
    for (int i = 0; i < wholeLength / 2; i++) {
        char temp = bufferChar[i];
        bufferChar[i] = bufferChar[wholeLength - i - 1];
        bufferChar[wholeLength - i - 1] = temp;
    }
    
    if (precision > 0) {
        // Add the decimal point
        bufferChar[wholeLength++] = '.';
        
        // Convert the fractional part to a string with the desired precision
        for (int i = 0; i < precision; i++) {
            fractionalPart *= 10;
            int digit = (int)fractionalPart;
            bufferChar[wholeLength++] = '0' + digit;
            fractionalPart -= digit;
        }
    }
    
    // Null-terminate the string
    bufferChar[wholeLength] = '\0';
}

uint16_t floatToUint16(float floatValue) {
    // Multiply the float value by 100 to preserve two decimal places
    float multipliedValue = floatValue * 100.0f;
    
    // Cast the multiplied value to uint16_t
    uint16_t uintValue = (uint16_t)multipliedValue;

    return uintValue;
}

uint16_t returnTempData(int val) {
    if (val == 1) {
        return floatToUint16(temperatures[0]);
    } else if (val == 2) {
        return floatToUint16(temperatures[1]);
    } else if (val == 3) {
        return floatToUint16(temperatures[2]);
    } else if (val == 4) {
        return floatToUint16(temperatures[3]);
    } else {
        return 0.00;
    }
}

long long returnSDData(int val) {
    if (val == 1) {
        return value1;
    } else if (val == 2) {
        return value2;
    } else {
        return 0;
    }
}

char* returnCharBuffer() {
    return bufferChar;
}