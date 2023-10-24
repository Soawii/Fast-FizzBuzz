# FastFizzBuzz
Fast output for the first 10^9 lines of FizzBuzz

# Sources
Speeding up Linux pipes: https://mazzo.li/posts/fast-pipes.html  
Rewriting your code in machine code and running it (or Just-In-Time compilation): https://eli.thegreenplace.net/2013/11/05/how-to-jit-an-introduction

## Build
It *should* work with this:  
```
git clone https://github.com/Soawii/FastFizzBuzz  
cd FastFizzBuzz  
make  
make test  
```
If it doesn't:
```
git clone https://github.com/Soawii/FastFizzBuzz  
cd FastFizzBuzz  
gcc FizzBuzz.c -o FizzBuzz -O3 -march=native
time ./FizzBuzz | pv > /dev/null
```
## Algorithm explanation (step by step)
### The obvious solution
Let's start by implementing the most obvious solution to the problem and finding out what are the most time consuming parts of it.
We will iterate from 1 to 10^9 and check if the number is divisible by 3, 5 or by both, and output the corresponding string.
Here's the code for it:
```c
#include <stdio.h>
int main()
{
    for (int i = 1; i <= 1000000000; i++)
    {
        if (i % 3 == 0)
        {
            if (i % 5 == 0) printf("FizzBuzz\n");
            else printf("Fizz\n");
        }
        else if (i % 5 == 0) printf("Buzz\n");
        else printf("%i\n", i);
    }
    return 0;
}
```
### Speeding up the output
We can quickly find out that printing to stdout is by far taking the most of the program's time. But WHY is it so slow?   
The answer is stdout flushing. Each printf writes its contents into a temporary buffer which gets flushed and printed to the console just after a few calls.  
This is VERY innefective and we can fix it by implementing our own buffer with a much bigger size, writing to it instead of calling printf, and then outputting all of its contents into stdout when there's no more space.
```c
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
```
This runs almost three times faster than the previous program! However it is still very slow.  
### Speeding up the algorithm
#### Replacing sprintf() with memcpy()
We have dealt with the output speed, now we have to deal with the speed of the algorithm itself.   
After some testing, we see that sprintf() function is very costly to be ran for every number and needs to be replaced.  
We can replace it with memcpy(), but for that we need to store our current number as a string, let's do that.  
```c
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
```
And that makes the program run 7 times faster!  
#### Removing MOD and unrolling loops
Now we can notice that the MOD (%) operation is costly and should be the next on the chopping block.  
We can remove it because "Fizz, Buzz, Number etc." repeats itself every 15 lines, so we can simply unroll the loop, let's do it.  
The one digit numbers can not fit into a 15-number cycle as there would be many unnecesary checks, we print them out first and start our cycle from 10.  
This change also helps us very nicely with the digit carry over, because now our cycles of 15 are always going to end at 99, 999, 9999... and we will not have to check if we went over digit limit.   
```c
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
```
This makes our code a little bit faster, more stable and helps us transfer to the next big improvement.  
#### Reduce memcpy() calls 
The main problem we are facing now is too many memcpy calls on small strings, this function works much better on the bigger-sized strings with less calls.  
But how can we achieve less calls?   
The first thing that comes to mind is to create a big string with some number of lines of FizzBuzz and memcpy it to the output buffer each cycle, let's try to achieve it.  
We need to create a string only once per every digit, because only the digit number changes the string size.  
Each cycle we will have to increase each number in the string by the line amount in the string, which is why it's the best to make a string of 30 lines, as it's much easier to add 30 to each number in the string than it is to add 15.
```c
#include <stdio.h>
#include <string.h>
char* number_ptrs[16];
const int table[] = { 4, 7, 2, 11, 2, 7, 12, 2, 12, 7, 2, 11, 2, 7, 12, 2 };   // number of chars to jump from one tens digit to another
int main()
{
	char buffer[15000];
	char* buffer_ptr = buffer, * end_ptr = buffer + sizeof(buffer);
	char output_string[500];
  	char *string_ptr = output_string;
	printf("1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\n");
	long long int base_number = 1, current_number = 10;
	for (int digits = 2; digits < 10; digits++)
	{
		int block_size = sprintf(string_ptr, "Buzz\n%lli1\nFizz\n%lli3\n%lli4\nFizzBuzz\n%lli6\n%lli7\nFizz\n%lli9\nBuzz\nFizz\n%lli2\n%lli3\nFizz\nBuzz\n%lli6\nFizz\n%lli8\n%lli9\nFizzBuzz\n%lli1\n%lli2\nFizz\n%lli4\nBuzz\nFizz\n%lli7\n%lli8\nFizz\n", base_number, base_number, base_number, base_number, base_number, base_number, base_number + 1, base_number + 1, base_number + 1, base_number + 1, base_number + 1, base_number + 2, base_number + 2, base_number + 2, base_number + 2, base_number + 2);
		memcpy(buffer_ptr, string_ptr, block_size);
		current_number += 30;
		buffer_ptr += block_size;
		base_number *= 10;
		char* temp = string_ptr;
		for (int i = 0; i < 16; i++) // filling an array of pointers to the numbers in the string
		{
			temp += table[i] + (digits - 1);
			number_ptrs[i] = temp;
		}
		long long int runs = 1;
		while (runs > 0)
		{
			long long int runs_to_fill_output = ((end_ptr - buffer_ptr) / block_size), runs_to_fill_digit = (((base_number * 10) - current_number) / 30);
			runs = runs_to_fill_output > runs_to_fill_digit ? runs_to_fill_digit : runs_to_fill_output; // calculating the number of 30 line outputs we can do in the current digit or remaining buffer size
			current_number += runs * 30;
			for (int i = 0; i < runs; i++)
			{
				for (int j = 0; j < 16; j++) // adding 30 to each of 16 numbers in the string
				{
					char* temp = number_ptrs[j];
					if (*temp < '7') *temp += 3;
					else
					{
						*temp-- -= 7;
						while (*temp == '9') *temp-- = '0';
						*temp += 1;
					}
				}
				memcpy(buffer_ptr, string_ptr, block_size);
				buffer_ptr += block_size;
			}
			if (runs > 0)
			{
				fwrite(buffer, 1, buffer_ptr - buffer, stdout);
				buffer_ptr = buffer;
			}
		}
	}
	return 0;
}
```
This makes out program another 2.5 times faster and it is *at the moment* the final iteration of the program.
~
