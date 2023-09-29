#include <stdio.h>
#include <string.h>
const char F[] = "Fizz\n", B[] = "Buzz\n", FB[] = "FizzBuzz\n";
int main()
{
    char buffer[50000];
    char *buffer_ptr = buffer, *end_ptr = buffer + sizeof(buffer);
    for (int i = 1; i <= 1000000000; i++)
    {
        if (end_ptr - buffer_ptr < 15)
        {
            fwrite(buffer, 1, buffer_ptr - buffer, stdout);
            buffer_ptr = buffer;
        }
        if (i % 3 == 0)
        {
            if (i % 5 == 0) 
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
        else if (i % 5 == 0)
        {
            memcpy(buffer_ptr, B, 5);
            buffer_ptr += 5;
        }
        else 
        {
            int n = sprintf(buffer_ptr, "%i\n", i);
            buffer_ptr += n;
        }
    }
    fwrite(buffer, 1, buffer_ptr - buffer, stdout);
    return 0;
}
