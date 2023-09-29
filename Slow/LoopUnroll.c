#include <stdio.h>
#include <string.h>
const char F[] = "Fizz\n", B[] = "Buzz\n", FB[] = "FizzBuzz\n";
#define INCREASE_NUMBER {for (int i = 9; i >= 0; i--) { if (number[i] == '9') number[i] = '0'; else {number[i]++; break;}}}
int main()
{
    char number[11] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '9', '\n'};
    char *number_ptr;
    char buffer[50000];
    char *buffer_ptr = buffer, *end_ptr = buffer + sizeof(buffer);
    long long int left = 10, right = 100;
    printf("1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\n");
    for (int digits = 2; digits < 10; digits++)
    {
        number_ptr = number + (10 - digits);
        for (; left < right; left += 15)
        {
            if (end_ptr - buffer_ptr < 150)
            {
                fwrite(buffer, 1, buffer_ptr - buffer, stdout);
                buffer_ptr = buffer;
            }
            INCREASE_NUMBER; memcpy(buffer_ptr, B, 5); buffer_ptr += 5;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, F, 5); buffer_ptr += 5;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, FB, 9); buffer_ptr += 9;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, F, 5); buffer_ptr += 5;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, B, 5); buffer_ptr += 5;
            INCREASE_NUMBER; memcpy(buffer_ptr, F, 5); buffer_ptr += 5;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, number_ptr, digits + 1); buffer_ptr += digits + 1;
            INCREASE_NUMBER; memcpy(buffer_ptr, F, 5); buffer_ptr += 5;
        }
        right *= 10;
    }
    fwrite(buffer, 1, buffer_ptr - buffer, stdout);
    return 0;
}
