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
This makes out program another 2.5 times faster.
### Speeding up Linux pipes
After the last improvement we *again* run into the problem of the output being our bottleneck, and now it's much harder to figure out how to make it faster. 
Fortunately, we are not the first people that run into this problem. Linux pipes and their speed were thoroughly researched by a person named Francesco Mazzoli and he has posted his result here: https://mazzo.li/posts/fast-pipes.html 
#### Problems
After reading his post we can see a lot of reasons why out current output is so slow.  
The main problems being:  
1. Each "fwrite" we do, we copy our output one extra time.  
2. The default pipe size is 64KiB, page size is 4KiB. When the pipe gets full, we need to wait until it gets fully read until writing to it again.
#### Solutions
##### 1. Using vmsplice() so we don't copy the output when its not needed
We can fix the first problem by replacing "fwrite()" with "vmsplice()" in out program which doesn't copy the output.
As i've read, this function is buggy and poorly documented, so we need to take some precautions to make it work properly:
1. This function returning doesn't mean that it has written the whole buffer, so we must not change the piped buffer and write to the additional one instead. We will interchange these 2 buffers after each function call to avoid producing incorrect output.
2. Pipe size should be equal to the buffer size (both 2 buffers should be the same size) so when we write buffer to the pipe, it always fills the whole pipe and the pipe switches to read mode.
##### 2. Making the pipe size bigger so we don't need to wait so much after each cycle
We can achieve this with function "fcntl()" with F_SETPIPE_SZ flag being used. The optimal size may vary on the system and needs to tweaked a bit.  
  
Let's implement these functions into our solution and see how much faster it becomes!
```c
#define _GNU_SOURCE
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
char* number_ptrs[16];
const int table[] = { 4, 7, 2, 11, 2, 7, 12, 2, 12, 7, 2, 11, 2, 7, 12, 2 };   // number of chars to jump from one tens digit to another
#define BUFFER_SIZE (1 << 20)
int main()
{
    fcntl(1, F_SETPIPE_SZ, BUFFER_SIZE);
	char buffer1[BUFFER_SIZE + 1024], buffer2[BUFFER_SIZE + 1024], *current_buffer = buffer1, *buffer_ptr = buffer1;
	int buffer_in_use = 0;
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
			long long int runs_to_fill_output = (((current_buffer + BUFFER_SIZE) - buffer_ptr) / block_size) + 1, runs_to_fill_digit = (((base_number * 10) - current_number) / 30);
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
			    int left_over = buffer_ptr - (current_buffer + BUFFER_SIZE);
			    struct iovec BUFVEC = { current_buffer, BUFFER_SIZE};
                while (BUFVEC.iov_len > 0) 
                {
                    int written = vmsplice(1, &BUFVEC, 1, 0);
                    BUFVEC.iov_base = ((char*)BUFVEC.iov_base) + written;
                    BUFVEC.iov_len -= written;
                }
                fwrite(buffer, 1, buffer_ptr - buffer, stdout);
                if (buffer_in_use == 0)
                {
                    memcpy(buffer2, current_buffer + BUFFER_SIZE, left_over);
                    current_buffer = buffer2;
                }
                else
                {
                    memcpy(buffer1, current_buffer + BUFFER_SIZE, left_over);
                    current_buffer = buffer1;
                }
                buffer_ptr = current_buffer + left_over;
			}
		}
	}
	return 0;
}
```
This makes the program run almost 2 times faster!
### Making use of SIMD intrinsics
#### Basics
Making use of this technology can make the code faster, but most of this is done by compiler already with high optimization like -O3.    
The main reason why we're switching to this is to easily translate our code into ASM code later with no unnecessary intructions.  
Let's start by implementing the most obvious solution: Represent out current number as a __m256i, each byte representing a digit ('0' - '9').  
If we represent each digit in numbers from 0 to 9 it gets very difficult to handle the carry, after some time we can figure out that it is the best to represent digits as 0 = 246, 9 = 255 (the highest value of 8-bit unsigned integer), so that when we add 1 to a '9' digit, the carry to the next digit is happenning with us not having to do anything (139 + 1 -> {247, 249, 255} + {0, 0, 1} = {247, 250, 0}).   
The only thing we have to do after that is change all the zeroes to 246.  
The next SIMD function that we are going to use a lot is _mm256_shuffle_epi8() or vpshufb which shuffles one __m256i based on the value of another. For example (assuming __m256i store 4 bytes each) _mm256_shuffle_epi8({246, 247, 248, 249}, {0, 1, 1, 3}) = {249, 248, 248, 246} 
#### Introding the bytecode
From now on we'll be using bytecode: we will first generate some kind of bytecode (for example : 0 1 4 5 -1 -2) for our program, with each byte representing some function/intructions that we will be doing.  
At first this might seem useless but it will help us a lot later!
#### Naive solution
Let's start by implementing the naive solution using SIMD: store the number in __m256i (in 0 = 246, 9 = 255 format), increase it by 1 each line, handle the carry when needed, and copy it to the buffer using the shuffle.
```c
#define _GNU_SOURCE
#include <stdio.h>
#include <immintrin.h>
#include <fcntl.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

int digits, CODE_SIZE;

__m256i number, shuffle_number, shuffle;
__m256i ONE, VEC_198, VEC_246;

const char Fizz[] = "Fizz\n", Buzz[] = "Buzz\n", FizzBuzz[] = "FizzBuzz\n";

int8_t bytecode[500], *bytecode_ptr = bytecode;

#define BUFFER_SIZE (1 << 20)

char buffer1[BUFFER_SIZE + 1024], buffer2[BUFFER_SIZE + 1024], *current_buffer = buffer1, *buffer_ptr = buffer1;
int buffer_in_use = 0;

void set_constants()
{
    ONE = _mm256_set_epi64x(0, 0, 0, 1);
    VEC_246 = _mm256_set1_epi8(246);
    VEC_246 = _mm256_bsrli_epi128(VEC_246, 1);
    VEC_198 = _mm256_set1_epi8(198);
    shuffle = _mm256_set1_epi8(255);
    shuffle = _mm256_slli_si256(shuffle, 1);
    shuffle = _mm256_add_epi8(shuffle, _mm256_set_epi64x(0, 0, 0, 15));
    shuffle = _mm256_slli_si256(shuffle, 1);
}

#define INCREASE {number = _mm256_add_epi64(number, ONE); shuffle_number = _mm256_shuffle_epi8(_mm256_sub_epi8(number, VEC_198), shuffle);}
#define INCREASE_CARRY {number = _mm256_add_epi64(number, ONE); number = _mm256_max_epu8(number, VEC_246); shuffle_number = _mm256_shuffle_epi8(_mm256_sub_epi8(number, VEC_198), shuffle);}


/*
Bytecode meaning
1: Copy fizz to the output buffer
2: Copy buzz to the output buffer
3: Copy fizzbuzz to the output buffer
4: Copy the current number to the output buffer
5: Increase the number by 1
6: Handle the carry
7: Copy '\n' to the output buffer
-1 to -9: move output buffer -1 * N bytes to the right (with N being eqaul to current byte in out bytecode)
*/

void generate_bytecode(int from, int to)
{
    for (int i = from; i < to; i += 10)
    {
        int boundary = to < i + 10 ? to : i + 10;
        for (int j = i; j < boundary; j++)
        {
            if (j % 3 == 0)
            {
                if (j % 5 == 0) 
                {
                    *bytecode_ptr++ = 3;
                    *bytecode_ptr++ = -9;
                }
                else 
                {
                    *bytecode_ptr++ = 1;
                    *bytecode_ptr++ = -5;
                }
            }
            else if (j % 5 == 0)
            {
                *bytecode_ptr++ = 2;
                *bytecode_ptr++ = -5;
            }
            else
            {
                *bytecode_ptr++ = 4;
                *bytecode_ptr++ = -1 * digits;
                *bytecode_ptr++ = 7;
            }
            *bytecode_ptr++ = 5;
        }
        *bytecode_ptr++ = 6;
    }
}

void interpret_bytecode()
{
    for (int i = 0; i < CODE_SIZE; i++)
    {
        int8_t c = bytecode[i];
        if (c == 1) memcpy(buffer_ptr, Fizz, 5);
        else if (c == 2) memcpy(buffer_ptr, Buzz, 5);
        else if (c == 3) memcpy(buffer_ptr, FizzBuzz, 9);
        else if (c == 4) _mm256_storeu_si256((__m256i*)buffer_ptr, shuffle_number);
        else if (c == 5) 
        {
            number = _mm256_add_epi64(number, ONE); 
            shuffle_number = _mm256_shuffle_epi8(_mm256_sub_epi8(number, VEC_198), shuffle);
        }
        else if (c == 6)
        {
            number = _mm256_max_epu8(number, VEC_246); 
            shuffle_number = _mm256_shuffle_epi8(_mm256_sub_epi8(number, VEC_198), shuffle);
        }
        else if (c == 7) *buffer_ptr++ = '\n';
        else buffer_ptr += -1 * (c);
    }
}

int main()
{
    printf("1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\n");
    set_constants();
    uint64_t line_number = 10, line_boundary = 100;
    for (digits = 2; digits < 10; digits++)
    {
        bytecode_ptr = bytecode;
        generate_bytecode(line_number, line_number + 30);
        CODE_SIZE = bytecode_ptr - bytecode;
        number = ONE;
        for (int i = 0; i < digits - 1; i++) number = _mm256_slli_si256(number, 1);
        number = _mm256_add_epi8(number, VEC_246);
        shuffle = _mm256_slli_si256(shuffle, 1);
        shuffle = _mm256_add_epi8(shuffle, _mm256_set_epi64x(0, 0, 0, digits - 1));
        int STRING_SIZE = 94 + (16 * digits);
        int RUNS, RUNS_TO_DIGIT, RUNS_TO_BUFFER;
        while(1)
        {
            RUNS_TO_DIGIT = (line_boundary - line_number) / 30, RUNS_TO_BUFFER = (((current_buffer + BUFFER_SIZE) - buffer_ptr) / STRING_SIZE) + 1;
            RUNS = RUNS_TO_DIGIT < RUNS_TO_BUFFER ? RUNS_TO_DIGIT : RUNS_TO_BUFFER;
            if (RUNS == 0) break;
            for (int i = 0; i < RUNS; i++) interpret_bytecode();
            line_number += RUNS * 30;
            if (buffer_ptr >= (current_buffer + BUFFER_SIZE))
            {
                int left_over = buffer_ptr - (current_buffer + BUFFER_SIZE);
			    struct iovec BUFVEC = { current_buffer, BUFFER_SIZE};
                while (BUFVEC.iov_len > 0) 
                {
                    int written = vmsplice(1, &BUFVEC, 1, 0);
                    BUFVEC.iov_base = ((char*)BUFVEC.iov_base) + written;
                    BUFVEC.iov_len -= written;
                }
                //fwrite(current_buffer, 1, buffer_ptr - current_buffer, stdout);
                if (buffer_in_use == 0)
                {
                    memcpy(buffer2, current_buffer + BUFFER_SIZE, left_over);
                    current_buffer = buffer2;
                }
                else
                {
                    memcpy(buffer1, current_buffer + BUFFER_SIZE, left_over);
                    current_buffer = buffer1;
                }
                buffer_ptr = current_buffer + left_over;
            }
        }
        line_boundary *= 10;
    }
    fwrite(current_buffer, 1, buffer_ptr - current_buffer, stdout);
    return 0;
}
```
In this solution we can most of the functions that we will use in the faster variant.
#### Introding the bytecode and going faster
Now we run into the same problem as we had in the beginning: too many memcpy calls and too many number increments (most of them could be hard-coded)  
We can fix it in the same way as we did before: create a big string and perform functions on it. However, now our string would be a __m256i array, each index holding 32 bytes.
