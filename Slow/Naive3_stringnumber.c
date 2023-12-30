#include <stdio.h>
#include <string.h>
const char F[] = "Fizz\n", B[] = "Buzz\n", FB[] = "FizzBuzz\n";
int main()
{
    char number[11] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '\n'};
    char *number_ptr;
    char buffer[50000];
    char *buffer_ptr = buffer, *end_ptr = buffer + sizeof(buffer);
    long long int left = 1, right = 10;
    for (int digits = 1; digits < 10; digits++)
    {
        number_ptr = number + (10 - digits);
        for (; left < right; left++)
        {
            for (int i = 9; i >= 0; i--) // increasing the number by 1
            {
                if (number[i] == '9') number[i] = '0';
                else 
                {
                    number[i]++; 
                    break;
                }
            }
            if (end_ptr - buffer_ptr < 15)
            {
                fwrite(buffer, 1, buffer_ptr - buffer, stdout);
                buffer_ptr = buffer;
            }
            if (left % 3 == 0)
            {
                if (left % 5 == 0) 
                {
                    memcpy(buffer_ptr, FB, 9);
                    buffer_ptr += 9;
                }
                else 
                {
                    memcpy(buffer_ptr, F, 5);
                    buffer_ptr += 5;
                }
            }
            else if (left % 5 == 0)
            {
              memcpy(buffer_ptr, B, 5);
              buffer_ptr += 5;
            }
            else
            {
              memcpy(buffer_ptr, number_ptr, digits + 1);
              buffer_ptr += digits + 1;
            }
        }
        right *= 10;
    }
    fwrite(buffer, 1, buffer_ptr - buffer, stdout);
    return 0;
}
