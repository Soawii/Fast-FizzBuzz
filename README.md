# FastFizzBuzz
Fast output for the first 10^9 lines of FizzBuzz, output redirected to /dev/null
## Why was this made?
Was a challenge in my university to make a FizzBuzz problem produce 10^9 lines of output in less then 0.1 seconds. I found it fun and decided to try it out!
## Stats
| Naive | Naive2_Buffer | Naive3_StringNumber | Naive4_LoopUnroll |
| :---: | :----------: 	| :-----------------: | :---------------: |
| 57.3s | 44.4s         | 5.2s                | 4.6s	   	  |

| Intrinsics1 | Intrinsics2 | Intrinsics3_SingleThreaded   | FizzBuzz (Intrinsics3_MultiThreaded) |
| :---------: | :---------: |	 :----------------------:  | :------: |
| 11.2s       | 0.74s       |	 0.254s			   |  0.087s  |
# Build
Compile with this:
```
gcc FizzBuzz.c -o FizzBuzz -pthread -mavx2 -no-pie -march=native
```
Run:
```
./FizzBuzz > /dev/null
```
# Short algorithm explanation
We are first making a very fast single-threaded program, which is fast because of SIMD usage and translating our algorithm into machine code. Then we are multi-threading it to make the fastest version of the program.
# Algorithm explanation (with every major speed-up)
1. [Making the fast version of the program with the common headers](#the-obvious-solution)  
2. [Making use of SIMD intrinsics and bytecode](#making-use-of-simd-intrinsics)  
3. [Turning our code into opcode or Just-In-Time compilation](#turning-our-code-into-opcode-or-just-in-time-compilation)
4. [Multi-threading and the final solution](#multi-threading)
## The obvious solution
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

## Speeding up the output
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
## Speeding up the algorithm
### Replacing sprintf() with memcpy()
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
### Removing MOD and unrolling loops
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
### Reducing memcpy() calls 
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

## Making use of SIMD intrinsics
### Basics
Making use of this technology can make the code faster, but most of this is done by compiler already with high optimization like -O3.    
The main reason why we're switching to this is to easily translate our code into ASM code later with no unnecessary intructions.   
   
Let's start by implementing the most obvious solution: Represent out current number as a __m256i, each byte representing a digit ('0' - '9').  
If we represent each digit in numbers from 0 to 9 it gets very difficult to handle the carry, after some time we can figure out that it is the best to represent digits as 0 = 246, 9 = 255 (the highest value of 8-bit unsigned integer), so that when we add 1 to a '9' digit, the carry to the next digit is happenning with us not having to do anything (139 + 1 -> {247, 249, 255} + {0, 0, 1} = {247, 250, 0}).   
The only thing we have to do after that is change all the zeroes to 246.  
  
The next SIMD function that we are going to use a lot is _mm256_shuffle_epi8() or vpshufb which shuffles one __m256i based on the value of another. For example (assuming __m256i store 4 bytes each) _mm256_shuffle_epi8({246, 247, 248, 249}, {0, 1, 1, 3}) = {249, 248, 248, 246} 
### Introducing the bytecode
From now on we'll be using bytecode: we will first generate some kind of bytecode (for example : 0 1 4 5 -1 -2) for our program, with each byte representing some function/intructions that we will be doing.  
At first this might seem useless but it will help us a lot later!
### Naive solution
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
  
Now we run into the same problem as we had in the beginning: too many memcpy calls and too many number increments (most of them could be hard-coded)    
We can fix it in the same way as we did before: create a big string and perform functions on it. However, now our string would be a __m256i array, each index holding 32 bytes.  
### "String" representation
The best way to represent a string that i've found is: 
1. represent characters with their negative ASCII value (this will include all Fizz, Buzz characters and out hard-coded least significant digit)
2. everything else will be the position of a digit inside a number  
Example of this representation: 122\nFizz\n124\n -> {1, 0, -'2', -'\n', -'F', -'i', -'z', -'z', -'\n', 1, 0, -'4', -'\n' ...}  
  
We will store one __m256i (let's call it number) with the tens-billions (we hard-code least significant digit into the string so that we don't have to increment it so often) digits of the number (represented in 246 format), another __m256i (ascii_number) that is equal to the (__m256i number) with each byte decreased by 198 so that 246 turns into 48 (which is ascii value of '0'), 247 into 49 (which is '1') etc.  
This representation makes it possible for us to do the following: shuffle (__m256i ascii_number) by this string (or _mm256_shuffle_epi8(ascii_number, *this string*)). This makes our string to become 0 at places with negative values (because their most significant bit is 1), and at other places it will replace the indexes with the values in ascii_number.  
Example: 122\nFizz\n124\n -> {1, 0, -'2', -'\n', -'F', -'i', -'z', -'z', -'\n', 1, 0, -'4', -'\n' ...} -> {'1', '2', 0, 0, 0, 0, 0, 0, 0, '1', '2', 0, 0 ...}
  
So now we have a string with ten->billions digits being in place, and to produce another chars the only thing we have to do is substruct the original string from the produced one (byte by byte).   
So : {'1', '2', 0, 0, 0, 0, 0, 0, 0, '1', '2', 0, 0 ...} - {1, 0, -'2', -'\n', -'F', -'i', -'z', -'z', -'\n', 1, 0, -'4', -'\n' ...} = {'0', '2', '2', '\n', 'F', 'i', 'z', 'z', '\n', '0', '2', '4', '\n' ...}.  
Now we have a little problem, we can see that hundreds-digit is incorrect since it gets decreased by 1, and all digits after (thousands->billions) will also be incorrect because of that.  
We can fix this by changing the previously-mentioned 198 offset, now we want it to be 1 bigger for each more significant digit so that out operations lead to the correct result. This will lead to out ascii-number being bigger by 1 in hundreds-digit, bigger by 2 in thousands-digit, bigger by 3 in tens-thousands-digit etc.  
So now if we take the previous example it would now work like this:  
1. (do the shuffle) {1, 0, -'2', -'\n', -'F', -'i', -'z', -'z', -'\n', 1, 0, -'4', -'\n' ...} -> {'2', '2', 0, 0, 0, 0, 0, 0, 0, '2', '2', 0, 0 ...} (hundreds-digit ascii value is biffer by 1)
2. (substract byte-by-byte) {'2', '2', 0, 0, 0, 0, 0, 0, 0, '2', '2', 0, 0 ...} ->  {'1', '2', '2', '\n', 'F', 'i', 'z', 'z', '\n', '1', '2', '4', '\n' ...}.  
And now we have the correct output!    
    
We will have to store this in array of __m256i (so each index holds 32 bytes). We will have to change the number each 10 lines, so some indexes must be smaller then 32 bytes so that we can increase the number after outputting the index and then go onto the next one.  
So our whole program now looks like this:
1. print first 10 lines
2. go from digits 2 to 9 (including)
3. generate the __m256i array for our current digit, generate the bytecode so that we know what to do
4. interpret the bytecode and run in until the digit end with some output inbetween  

Let's implement that.
```c
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>
#include <fcntl.h>

__m256i number, shuffle, ascii_number;
__m256i VEC_198, ONE, VEC_246;

__m256i shuffles[50]; 
uint8_t shuffle_idx = 0;

const char Fizz[] = "Fizz\n", Buzz[] = "Buzz\n", FizzBuzz[] = "FizzBuzz\n";

int digits;

int8_t code[200], * code_ptr = code;

int CODE_SIZE;

#define PAGE_SIZE 4096
#define BUFFER_SIZE (1 << 20)

alignas(PAGE_SIZE) char buffer1[BUFFER_SIZE + 1024], buffer2[BUFFER_SIZE + 1024], * current_buffer = buffer1, * buffer_ptr = buffer1;
int buffer_in_use = 0;

char string[2000], * string_ptr;

void set_constants()
{
    memset(code, 0, sizeof(code));
    ONE = _mm256_set_epi64x(0, 1, 0, 1);
    VEC_246 = _mm256_set1_epi8(246);
    VEC_198 = _mm256_set_epi8(198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198, 198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198);
}

void interpret_bytecode()
{
    for (int i = 0; i < CODE_SIZE; i++)
    {
        int8_t c = code[i];
        if (c == 1)
        {
            shuffle = _mm256_shuffle_epi8(ascii_number, shuffles[shuffle_idx]);
            shuffle = _mm256_sub_epi8(shuffle, shuffles[shuffle_idx]);
            _mm256_storeu_si256((__m256i*)buffer_ptr, shuffle);
            buffer_ptr += 32;
            shuffle_idx++;
        }
        else if (c == 2)
        {
            number = _mm256_add_epi64(number, ONE);
            number = _mm256_max_epu8(number, VEC_246);
            ascii_number = _mm256_sub_epi8(number, VEC_198);
        }
        else
        {
            buffer_ptr += c;
        }
    }
    shuffle_idx = 0;
}

void fill_shuffles(int from, int to)
{
    for (int i = from; i < to; i += 32)
    {
        int boundary = to < i + 32 ? to : i + 32;
        for (int j = i; j < boundary; j++)
        {
            int temp_idx = (j - i);
            __m256i temp_xmm;
            int8_t val = string[j] < 10 ? string[j] : ((int8_t)((int8_t)-1 * (int8_t)string[j]));
            if (temp_idx < 16) temp_xmm = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, val);
            else
            {
                temp_xmm = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, val, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                temp_idx -= 16;
            }
            for (int k = 0; k < temp_idx; k++) temp_xmm = _mm256_bslli_epi128(temp_xmm, 1);
            shuffles[shuffle_idx] = _mm256_or_si256(shuffles[shuffle_idx], temp_xmm);
        }
        *code_ptr++ = 1;
        if (boundary != i + 32) *code_ptr++ = to - (i + 32);
        shuffle_idx++;
    }
    *code_ptr++ = 2;
}

const char FIRST_TEN[] = "1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\n";

int main()
{
    fcntl(1, F_SETPIPE_SZ, BUFFER_SIZE);
    set_constants();
    uint64_t line_number = 10, line_boundary = 100;
    memcpy(buffer_ptr, FIRST_TEN, strlen(FIRST_TEN));
    buffer_ptr += strlen(FIRST_TEN);
    for (digits = 2; digits < 4; digits++)
    {
        for (int i = 0; i < 50; i++) shuffles[i] = _mm256_set1_epi8(0);
        string_ptr = string;
        uint8_t shuffle_init[] = { 9, 8, 7, 6, 5, 4, 3, 2, 1, 0, '0', '\n' }, * shuffle_init_ptr = shuffle_init + 11 - digits;
        for (int i = 10; i < 40; i++)
        {
            if (i % 3 == 0)
            {
                if (i % 5 == 0)
                {
                    memcpy(string_ptr, FizzBuzz, 9);
                    string_ptr += 9;
                }
                else
                {
                    memcpy(string_ptr, Fizz, 5);
                    string_ptr += 5;
                }
            }
            else if (i % 5 == 0)
            {
                memcpy(string_ptr, Buzz, 5);
                string_ptr += 5;
            }
            else
            {
                memcpy(string_ptr, shuffle_init_ptr, digits + 1);
                string_ptr += digits + 1;
            }
            if (shuffle_init[10] == '9') shuffle_init[10] = '0';
            else shuffle_init[10]++;
        }
        code_ptr = code;
        const int FIRST_BOUNDARY = 30 + (6 * digits), SECOND_BOUNDARY = 60 + (11 * digits), THIRD_BOUNDARY = 94 + (16 * digits);
        fill_shuffles(0, FIRST_BOUNDARY);
        fill_shuffles(FIRST_BOUNDARY, SECOND_BOUNDARY);
        fill_shuffles(SECOND_BOUNDARY, THIRD_BOUNDARY);
        CODE_SIZE = code_ptr - code;
        shuffle_idx = 0;
        number = ONE;
        for (int i = 0; i < digits - 2; i++) number = _mm256_slli_si256(number, 1);
        number = _mm256_add_epi8(number, VEC_246);
        ascii_number = _mm256_sub_epi8(number, VEC_198);
        uint64_t RUNS, RUNS_TO_DIGIT, RUNS_TO_BUFFER;
        while (1)
        {
            RUNS_TO_DIGIT = (line_boundary - line_number) / 30;
            RUNS_TO_BUFFER = ((current_buffer + BUFFER_SIZE) - buffer_ptr) / THIRD_BOUNDARY + 1;
            RUNS = RUNS_TO_BUFFER < RUNS_TO_DIGIT ? RUNS_TO_BUFFER : RUNS_TO_DIGIT;
            if (RUNS == 0) break;
            for (int i = 0; i < RUNS; i++) interpret_bytecode();
            if (buffer_ptr >= current_buffer + BUFFER_SIZE)
            {
                 struct iovec BUFVEC = { current_buffer, BUFFER_SIZE };
                 while (BUFVEC.iov_len > 0)
                 {
                     int written = vmsplice(1, &BUFVEC, 1, 0);
                     BUFVEC.iov_base = ((char*)BUFVEC.iov_base) + written;
                     BUFVEC.iov_len -= written;
                 }
                //fwrite(current_buffer, 1, BUFFER_SIZE, stdout);
                int leftover = buffer_ptr - (current_buffer + BUFFER_SIZE);
                if (buffer_in_use == 0)
                {
                    memcpy(buffer2, current_buffer + BUFFER_SIZE, leftover);
                    current_buffer = buffer2;
                }
                else
                {
                    memcpy(buffer1, current_buffer + BUFFER_SIZE, leftover);
                    current_buffer = buffer1;
                }
                buffer_in_use = !buffer_in_use;
                buffer_ptr = current_buffer + leftover;
            }
            line_number += RUNS * 30;
        }
        line_boundary *= 10;
    }
    fwrite(current_buffer, 1, buffer_ptr - current_buffer, stdout);
    printf("1000000000\n");
    return 0;
}
```
### Hard-coding hundreds-digit
Looks good! However we still increment the number way too often, so we could hardcode the hundreds-digit into the string as well. This doesn't change our code that much but still increases its performance.  
Below is only the part of the code because everything else is exactly the same as in the previous program.  
 ```c
const char FIRST_100_LINES[] = "1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz\n16\n17\nFizz\n19\nBuzz\nFizz\n22\n23\nFizz\nBuzz\n26\nFizz\n28\n29\nFizzBuzz\n31\n32\nFizz\n34\nBuzz\nFizz\n37\n38\nFizz\nBuzz\n41\nFizz\n43\n44\nFizzBuzz\n46\n47\nFizz\n49\nBuzz\nFizz\n52\n53\nFizz\nBuzz\n56\nFizz\n58\n59\nFizzBuzz\n61\n62\nFizz\n64\nBuzz\nFizz\n67\n68\nFizz\nBuzz\n71\nFizz\n73\n74\nFizzBuzz\n76\n77\nFizz\n79\nBuzz\nFizz\n82\n83\nFizz\nBuzz\n86\nFizz\n88\n89\nFizzBuzz\n91\n92\nFizz\n94\nBuzz\nFizz\n97\n98\nFizz\n";

int main()
{
    fcntl(1, F_SETPIPE_SZ, BUFFER_SIZE);
    set_constants();
    memcpy(buffer_ptr, FIRST_100_LINES, strlen(FIRST_100_LINES));
    buffer_ptr += strlen(FIRST_100_LINES);
    uint64_t line_number = 100, line_boundary = 1000;
    for (digits = 3; digits < 10; digits++)
    {
        for (int i = 0; i < 500; i++) shuffles[i] = _mm256_set1_epi8(0);
        string_ptr = string;
        uint8_t shuffle_init[] = { 8, 7, 6, 5, 4, 3, 2, 1, 0, '0', '0', '\n' }, * shuffle_init_ptr = shuffle_init + 11 - digits;
        for (int i = 100; i < 400; i += 10)
        {
            for (int j = i; j < i + 10; j++)
            {
                if (j % 3 == 0)
                {
                    if (j % 5 == 0)
                    {
                        memcpy(string_ptr, FizzBuzz, 9);
                        string_ptr += 9;
                    }
                    else
                    {
                        memcpy(string_ptr, Fizz, 5);
                        string_ptr += 5;
                    }
                }
                else if (j % 5 == 0)
                {
                    memcpy(string_ptr, Buzz, 5);
                    string_ptr += 5;
                }
                else
                {
                    memcpy(string_ptr, shuffle_init_ptr, digits + 1);
                    string_ptr += digits + 1;
                }
                if (shuffle_init[10] == '9') shuffle_init[10] = '0';
                else shuffle_init[10]++;
            }
            if (shuffle_init[9] == '9') shuffle_init[9] = '0';
            else shuffle_init[9]++;
        }
        code_ptr = code;
        const int FIRST_BOUNDARY = 312 + (54 * digits), SECOND_BOUNDARY = 624 + (107 * digits), THIRD_BOUNDARY = 940 + (160 * digits);
        fill_shuffles(0, FIRST_BOUNDARY);
        fill_shuffles(FIRST_BOUNDARY, SECOND_BOUNDARY);
        fill_shuffles(SECOND_BOUNDARY, THIRD_BOUNDARY);
        CODE_SIZE = code_ptr - code;
        shuffle_idx = 0;
        number = ONE;
        for (int i = 0; i < digits - 3; i++) number = _mm256_slli_si256(number, 1);
        number = _mm256_add_epi8(number, VEC_246);
        shuffle_number = _mm256_sub_epi8(number, VEC_198);
        uint64_t RUNS, RUNS_TO_DIGIT = (line_boundary - line_number) / 300, RUNS_TO_BUFFER;
        while (1)
        {
            RUNS_TO_BUFFER = ((current_buffer + BUFFER_SIZE) - buffer_ptr) / THIRD_BOUNDARY + 1;
            RUNS = RUNS_TO_BUFFER < RUNS_TO_DIGIT ? RUNS_TO_BUFFER : RUNS_TO_DIGIT;
            if (RUNS == 0) break;
            for (int i = 0; i < RUNS; i++) interpret_bytecode();
            if (buffer_ptr >= current_buffer + BUFFER_SIZE)
            {
                struct iovec BUFVEC = { current_buffer, BUFFER_SIZE};
                while (BUFVEC.iov_len > 0)
                {
                    int written = vmsplice(1, &BUFVEC, 1, 0);
                    BUFVEC.iov_base = ((char*)BUFVEC.iov_base) + written;
                    BUFVEC.iov_len -= written;
                }
                //fwrite(current_buffer, 1, BUFFER_SIZE, stdout);
                int leftover = buffer_ptr - (current_buffer + BUFFER_SIZE);
                if (buffer_in_use == 0)
                {
                    memcpy(buffer2, current_buffer + BUFFER_SIZE, leftover);
                    current_buffer = buffer2;
                }
                else
                {
                    memcpy(buffer1, current_buffer + BUFFER_SIZE, leftover);
                    current_buffer = buffer1;
                }
                buffer_in_use = !buffer_in_use;
                buffer_ptr = current_buffer + leftover;
            }
            line_number += RUNS * 300;
            RUNS_TO_DIGIT -= RUNS;
        }
        line_boundary *= 10;
    }
    fwrite(current_buffer, 1, buffer_ptr - current_buffer, stdout);
    printf("1000000000\n");
    return 0;
}
```
## Turning our code into opcode or Just-In-Time compilation
We have increased the performance good enough to move onto the final improvement - turn this into opcode, push it into executable chuck of memory and run it to avoid extra intructions.  
This is the main reason why we introduced bytecode earlier, this makes it much easier to turn each bytecode byte into opcode and push it into memory.  
This technique is called Just-In-Time compilation ( https://eli.thegreenplace.net/2013/11/05/how-to-jit-an-introduction ), where the sensitive blocks of code get turned into bytecode and translated into machine code to improve performance.   
However, now we will implement it ourselves.  
Now, we can also see why we turned to SIMD, each of these functions is an intruction that is eaily translated into opcode.
### This is the fastest single-threaded solution that i could do, it works in 0.25 seconds on my PC.
```c
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>

__m256i number, shuffle, ascii_number;
__m256i ONE, VEC_198, VEC_246;

__m256i shuffles[1000];
uint8_t shuffle_idx = 0;

const char Fizz[] = "Fizz\n", Buzz[] = "Buzz\n", FizzBuzz[] = "FizzBuzz\n";

int digits;

uint8_t *opcode, *opcode_ptr;
typedef uint8_t* (*opcode_function)(uint8_t*, int);

int8_t bytecode[3000], * bytecode_ptr = bytecode;

int CODE_SIZE;

#define BUFFER_SIZE 262144
alignas(4096) char buffer[BUFFER_SIZE + 4096], * buffer_ptr = buffer;

char string[3000], * string_ptr;

void set_constants()
{
    memset(bytecode, 0, sizeof(bytecode));
    ONE = _mm256_set_epi64x(0, 1, 0, 1);
    VEC_246 = _mm256_set1_epi8(246);
    VEC_198 = _mm256_set_epi8(198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198, 198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198);
}

void generate_opcode()
{
    opcode_ptr = opcode;
    uint32_t offset = 0, rip_distance;
    __m256i* shuffles_ptr = shuffles;
    for (int i = 0; i < CODE_SIZE; i++)
    {
        int8_t c = bytecode[i];
        if (c == 1)
        {
            *opcode_ptr++ = 0xC5; *opcode_ptr++ = 0x7D; *opcode_ptr++ = 0x6F; *opcode_ptr++ = 0x35; // |
            rip_distance = (uint8_t*)(shuffles_ptr) - (opcode_ptr + 4);                             // |
            memcpy(opcode_ptr, &rip_distance, 4);                                                   // |
            opcode_ptr += 4;                                                                        // | = vmovdqa ymm14, YMMWORD PTR [shuffles_ptr] (or vmovdqa ymm14, YMMWORD PTR [rip + ((uint8_t*)(shuffles_ptr) - (uint8_t*)(opcode_ptr + 4))]) 
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x42; *opcode_ptr++ = 0x15; *opcode_ptr++ = 0x00; *opcode_ptr++ = 0xFE;   // vpshufb ymm15, ymm13, ymm14
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x05; *opcode_ptr++ = 0xF8; *opcode_ptr++ = 0xFE;   // vpsubb ymm15, ymm15, ymm14
            *opcode_ptr++ = 0xC5; *opcode_ptr++ = 0x7E; *opcode_ptr++ = 0x7F; *opcode_ptr++ = 0xBF; memcpy(opcode_ptr, &offset, 4); opcode_ptr += 4;  // vmovdqu YMMWORD PTR [rdi + offset], ymm15
            offset += 32;
            shuffles_ptr++;
        }
        else if (c == 2)
        {
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x35; *opcode_ptr++ = 0xD4; *opcode_ptr++ = 0xCA; // vpaddq ymm9, ymm9, ymm10
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x35; *opcode_ptr++ = 0xDE; *opcode_ptr++ = 0xCC; // vpmaxub ymm9, ymm9, ymm12
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x35; *opcode_ptr++ = 0xF8; *opcode_ptr++ = 0xEB; // vpsubb ymm13, ymm9, ymm11
        }
        else offset += c;
    }
    *opcode_ptr++ = 0x48; *opcode_ptr++ = 0x81; *opcode_ptr++ = 0xC7; memcpy(opcode_ptr, &offset, 4); opcode_ptr += 4; // add rdi, offset
    *opcode_ptr++ = 0xFF; *opcode_ptr++ = 0xCE; // dec esi
    *opcode_ptr++ = 0x0F; *opcode_ptr++ = 0x85;                             // |
    rip_distance = opcode - (opcode_ptr + 4);                               // |
    memcpy(opcode_ptr, &rip_distance, 4);                                   // |          
    opcode_ptr += 4;                                                        // | = jnz opcode
    *opcode_ptr++ = 0xC5; *opcode_ptr++ = 0x7D; *opcode_ptr++ = 0x7F; *opcode_ptr++ = 0x0D; // |
    rip_distance = (uint8_t*)(&number) - (opcode_ptr + 4);                                  // |             
    memcpy(opcode_ptr, &rip_distance, 4);                                                   // |                            
    opcode_ptr += 4;                                                                        // | = vmovdqa YMMWORD PTR [rip + number], ymm9
    *opcode_ptr++ = 0x48; *opcode_ptr++ = 0x89; *opcode_ptr++ = 0xF8; // mov rax, rdi
    *opcode_ptr++ = 0xC3; // ret
}

void fill_shuffles(int from, int to)
{
    for (int i = from; i < to; i += 32)
    {
        int boundary = to < i + 32 ? to : i + 32;
        for (int j = i; j < boundary; j++)
        {
            int temp_idx = (j - i);
            __m256i temp_xmm;
            int8_t val = string[j] < 10 ? string[j] : ((int8_t)((int8_t)-1 * (int8_t)string[j]));
            if (temp_idx < 16) temp_xmm = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, val);
            else
            {
                temp_xmm = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, val, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                temp_idx -= 16;
            }
            for (int k = 0; k < temp_idx; k++) temp_xmm = _mm256_bslli_epi128(temp_xmm, 1);
            shuffles[shuffle_idx] = _mm256_or_si256(shuffles[shuffle_idx], temp_xmm);
        }
        *bytecode_ptr++ = 1;
        if (boundary != i + 32) *bytecode_ptr++ = to - (i + 32);
        shuffle_idx++;
    }
    *bytecode_ptr++ = 2;
}

const char FIRST_100_LINES[] = "1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz\n16\n17\nFizz\n19\nBuzz\nFizz\n22\n23\nFizz\nBuzz\n26\nFizz\n28\n29\nFizzBuzz\n31\n32\nFizz\n34\nBuzz\nFizz\n37\n38\nFizz\nBuzz\n41\nFizz\n43\n44\nFizzBuzz\n46\n47\nFizz\n49\nBuzz\nFizz\n52\n53\nFizz\nBuzz\n56\nFizz\n58\n59\nFizzBuzz\n61\n62\nFizz\n64\nBuzz\nFizz\n67\n68\nFizz\nBuzz\n71\nFizz\n73\n74\nFizzBuzz\n76\n77\nFizz\n79\nBuzz\nFizz\n82\n83\nFizz\nBuzz\n86\nFizz\n88\n89\nFizzBuzz\n91\n92\nFizz\n94\nBuzz\nFizz\n97\n98\nFizz\n";

int main()
{
    set_constants();
    opcode = (uint8_t*)mmap(NULL, 10000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON | MAP_32BIT, -1, 0);
    memcpy(buffer_ptr, FIRST_100_LINES, strlen(FIRST_100_LINES));
    buffer_ptr += strlen(FIRST_100_LINES);
    uint64_t line_number = 100, line_boundary = 1000;
    for (digits = 3; digits < 10; digits++)
    {
        for (int i = 0; i < 500; i++) shuffles[i] = _mm256_setzero_si256();
        string_ptr = string;
        uint8_t shuffle_init[] = { 8, 7, 6, 5, 4, 3, 2, 1, 0, '0', '0', '\n' }, * shuffle_init_ptr = shuffle_init + 11 - digits;
        for (int i = 100; i < 400; i += 10)
        {
            for (int j = i; j < i + 10; j++)
            {
                if (j % 3 == 0)
                {
                    if (j % 5 == 0)
                    {
                        memcpy(string_ptr, FizzBuzz, 9);
                        string_ptr += 9;
                    }
                    else
                    {
                        memcpy(string_ptr, Fizz, 5);
                        string_ptr += 5;
                    }
                }
                else if (j % 5 == 0)
                {
                    memcpy(string_ptr, Buzz, 5);
                    string_ptr += 5;
                }
                else
                {
                    memcpy(string_ptr, shuffle_init_ptr, digits + 1);
                    string_ptr += digits + 1;
                }
                if (shuffle_init[10] == '9') shuffle_init[10] = '0';
                else shuffle_init[10]++;
            }
            if (shuffle_init[9] == '9') shuffle_init[9] = '0';
            else shuffle_init[9]++;
        }
        bytecode_ptr = bytecode;
        const int FIRST_BOUNDARY = 312 + (54 * digits), SECOND_BOUNDARY = 624 + (107 * digits), THIRD_BOUNDARY = 940 + (160 * digits);
        fill_shuffles(0, FIRST_BOUNDARY);
        fill_shuffles(FIRST_BOUNDARY, SECOND_BOUNDARY);
        fill_shuffles(SECOND_BOUNDARY, THIRD_BOUNDARY);
        CODE_SIZE = bytecode_ptr - bytecode;
        shuffle_idx = 0;
        number = ONE;
        for (int i = 0; i < digits - 3; i++) number = _mm256_slli_si256(number, 1);
        number = _mm256_add_epi8(number, VEC_246);
        ascii_number = _mm256_sub_epi8(number, VEC_198);
        generate_opcode();
        opcode_function f = opcode;
        uint64_t RUNS, RUNS_TO_DIGIT = (line_boundary - line_number) / 300, RUNS_TO_BUFFER;
        while (1)
        {
            RUNS_TO_BUFFER = ((buffer + BUFFER_SIZE) - buffer_ptr) / THIRD_BOUNDARY + 1;
            RUNS = RUNS_TO_BUFFER < RUNS_TO_DIGIT ? RUNS_TO_BUFFER : RUNS_TO_DIGIT;
            if (RUNS == 0) break;
            asm("vmovdqa %0, %%ymm10\n\t"
                "vmovdqa %1, %%ymm11\n\t"
                "vmovdqa %2, %%ymm12\n\t"
                "vmovdqa %3, %%ymm9\n\t"
                "vpsubb %%ymm11, %%ymm9, %%ymm13"
                :
            : "" (ONE),
                "" (VEC_198),
                "" (VEC_246),
                "" (number));
            buffer_ptr = f(buffer_ptr, RUNS);
            if (buffer_ptr > buffer + BUFFER_SIZE)
            {
                int left_over = buffer_ptr - (buffer + BUFFER_SIZE);
                fwrite(buffer, 1, BUFFER_SIZE, stdout); 
                memcpy(buffer, buffer + BUFFER_SIZE, left_over);
                buffer_ptr = buffer + left_over;
            }
            line_number += RUNS * 300;
            RUNS_TO_DIGIT -= RUNS;
        }
        line_boundary *= 10;
    }
    fwrite(buffer, 1, buffer_ptr - buffer, stdout);
    fprintf(stderr, "1000000000\n");
    return 0;
}
```
## Multi-threading 
Now that we made a really fast single-threaded solution, we can finally move to multi-threading. This program isn't too dificult to multi-thread, each thread has to simply produce output for a certain amount of lines, wait for each other thread to finish, and output everything in order.  
### This is the final implementation, it works in 0.08 seconds, but can be much faster on a better PC that can make use of more threads.
```c
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>
#include <pthread.h>
 
#define LINES_PER_THREAD 450000 // has to be a multiple of 300, change it up to fit your PC better
#define NUM_THREADS 4 // change according to your PC

__m256i ONE, VEC_198, VEC_246;
 
static __m256i shuffles[1000];
uint8_t shuffle_idx = 0;
 
const char Fizz[] = "Fizz\n", Buzz[] = "Buzz\n", FizzBuzz[] = "FizzBuzz\n";
 
int digits;
 
uint8_t *opcode, *opcode_ptr;
typedef uint8_t* (*opcode_function)(uint8_t*, int);
static opcode_function opcode_exec;
 
int8_t bytecode[3000], * bytecode_ptr = bytecode;
int CODE_SIZE;
 
char string[3000], * string_ptr;
 
void set_constants()
{
    memset(bytecode, 0, sizeof(bytecode));
    ONE = _mm256_set_epi64x(0, 1, 0, 1);
    VEC_246 = _mm256_set1_epi8(246);
    VEC_198 = _mm256_set_epi8(198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198, 198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198);
}
 
void generate_opcode()
{
    opcode_ptr = opcode;
    uint32_t offset = 0, rip_distance;
    __m256i* shuffles_ptr = shuffles;
    for (int i = 0; i < CODE_SIZE; i++)
    {
        int8_t c = bytecode[i];
        if (c == 1)
        {
            *opcode_ptr++ = 0xC5; *opcode_ptr++ = 0x7D; *opcode_ptr++ = 0x6F; *opcode_ptr++ = 0x35; // |
            rip_distance = (uint8_t*)(shuffles_ptr) - (opcode_ptr + 4);                             // |
            memcpy(opcode_ptr, &rip_distance, 4);                                                   // |
            opcode_ptr += 4;                                                                        // | = vmovdqa ymm14, YMMWORD PTR [shuffles_ptr] (or vmovdqa ymm14, YMMWORD PTR [rip + ((uint8_t*)(shuffles_ptr) - (uint8_t*)(opcode_ptr + 4))]) 
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x42; *opcode_ptr++ = 0x15; *opcode_ptr++ = 0x00; *opcode_ptr++ = 0xFE;   // vpshufb ymm15, ymm13, ymm14
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x05; *opcode_ptr++ = 0xF8; *opcode_ptr++ = 0xFE;   // vpsubb ymm15, ymm15, ymm14
            *opcode_ptr++ = 0xC5; *opcode_ptr++ = 0x7E; *opcode_ptr++ = 0x7F; *opcode_ptr++ = 0xBF; memcpy(opcode_ptr, &offset, 4); opcode_ptr += 4;  // vmovdqu YMMWORD PTR [rdi + offset], ymm15
            offset += 32;
            shuffles_ptr++;
        }
        else if (c == 2)
        {
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x35; *opcode_ptr++ = 0xD4; *opcode_ptr++ = 0xCA; // vpaddq ymm9, ymm9, ymm10
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x35; *opcode_ptr++ = 0xDE; *opcode_ptr++ = 0xCC; // vpmaxub ymm9, ymm9, ymm12
            *opcode_ptr++ = 0xC4; *opcode_ptr++ = 0x41; *opcode_ptr++ = 0x35; *opcode_ptr++ = 0xF8; *opcode_ptr++ = 0xEB; // vpsubb ymm13, ymm9, ymm11
        }
        else offset += c;
    }
    *opcode_ptr++ = 0x48; *opcode_ptr++ = 0x81; *opcode_ptr++ = 0xC7; memcpy(opcode_ptr, &offset, 4); opcode_ptr += 4; // add rdi, offset
    *opcode_ptr++ = 0xFF; *opcode_ptr++ = 0xCE; // dec esi
    *opcode_ptr++ = 0x0F; *opcode_ptr++ = 0x85; // |
    rip_distance = opcode - (opcode_ptr + 4);   // |
    memcpy(opcode_ptr, &rip_distance, 4);       // |          
    opcode_ptr += 4;                            // | = jnz opcode
    *opcode_ptr++ = 0xC3; // ret
}
 
void fill_shuffles(int from, int to)
{
    for (int i = from; i < to; i += 32)
    {
        int boundary = to < i + 32 ? to : i + 32;
        for (int j = i; j < boundary; j++)
        {
            int temp_idx = (j - i);
            __m256i temp_xmm;
            int8_t val = string[j] < 10 ? string[j] : ((int8_t)((int8_t)-1 * (int8_t)string[j]));
            if (temp_idx < 16) temp_xmm = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, val);
            else
            {
                temp_xmm = _mm256_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, val, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
                temp_idx -= 16;
            }
            for (int k = 0; k < temp_idx; k++) temp_xmm = _mm256_bslli_epi128(temp_xmm, 1);
            shuffles[shuffle_idx] = _mm256_or_si256(shuffles[shuffle_idx], temp_xmm);
        }
        *bytecode_ptr++ = 1;
        if (boundary != i + 32) *bytecode_ptr++ = to - (i + 32);
        shuffle_idx++;
    }
    *bytecode_ptr++ = 2;
}
 
typedef struct {
    char* thread_buffer;
    int buffer_len;
    int start_number;
    int end_number;
    int runs;
    int thread;
    pthread_spinlock_t work;
    pthread_mutex_t idle;
    __m256i number;
    char pad[86];
} arguments_struct;
 
#define MAX_DIGITS 10
#define BUFFER_SIZE (LINES_PER_THREAD / 300 * STRING_LEN + 1024)

void* thread_func(void* void_arguments)
{
    arguments_struct* ref_arguments = (arguments_struct*)void_arguments;
    while(1)
    {
        while (pthread_spin_trylock(&ref_arguments->work)) continue;
        arguments_struct arguments = *ref_arguments;
        const int start_number = arguments.start_number;
        arguments.number = _mm256_set_epi8(246, 246, 246, 246, 246, 246, 246, 246, 246, (start_number % 1000000000 / 100000000) + 246, (start_number % 100000000 / 10000000) + 246, (start_number % 10000000 / 1000000) + 246, (start_number % 1000000 / 100000) + 246, (start_number % 100000 / 10000) + 246, (start_number % 10000 / 1000) + 246, (start_number % 1000 / 100) + 246, 246, 246, 246, 246, 246, 246, 246, 246, 246, (start_number % 1000000000 / 100000000) + 246, (start_number % 100000000 / 10000000) + 246, (start_number % 10000000 / 1000000) + 246, (start_number % 1000000 / 100000) + 246, (start_number % 100000 / 10000) + 246, (start_number % 10000 / 1000) + 246, (start_number % 1000 / 100) + 246);
        asm("vmovdqa %0, %%ymm10\n\t"
                "vmovdqa %1, %%ymm11\n\t"
                "vmovdqa %2, %%ymm12\n\t"
                "vmovdqa %3, %%ymm9\n\t"
                "vpsubb %%ymm11, %%ymm9, %%ymm13\n\t"
                :
            : "" (ONE),
                "" (VEC_198),
                "" (VEC_246),
                "" (arguments.number));
        opcode_exec(arguments.thread_buffer, arguments.runs);
        pthread_mutex_unlock(&ref_arguments->idle);
    }
    pthread_exit(NULL);
}
 
const char FIRST_100_LINES[] = "1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz\n16\n17\nFizz\n19\nBuzz\nFizz\n22\n23\nFizz\nBuzz\n26\nFizz\n28\n29\nFizzBuzz\n31\n32\nFizz\n34\nBuzz\nFizz\n37\n38\nFizz\nBuzz\n41\nFizz\n43\n44\nFizzBuzz\n46\n47\nFizz\n49\nBuzz\nFizz\n52\n53\nFizz\nBuzz\n56\nFizz\n58\n59\nFizzBuzz\n61\n62\nFizz\n64\nBuzz\nFizz\n67\n68\nFizz\nBuzz\n71\nFizz\n73\n74\nFizzBuzz\n76\n77\nFizz\n79\nBuzz\nFizz\n82\n83\nFizz\nBuzz\n86\nFizz\n88\n89\nFizzBuzz\n91\n92\nFizz\n94\nBuzz\nFizz\n97\n98\nFizz\n";
 
int main()
{
    pthread_t threads[NUM_THREADS];
    alignas(256) arguments_struct thread_args[NUM_THREADS];
    for (int i = 0; i < NUM_THREADS; i++)
    {
        thread_args[i].thread = i;
        pthread_spin_init(&thread_args[i].work, PTHREAD_PROCESS_PRIVATE);
        pthread_spin_lock(&thread_args[i].work);
        pthread_mutex_init(&thread_args[i].idle, NULL);
        pthread_mutex_lock(&thread_args[i].idle);
        thread_args[i].thread_buffer = malloc((LINES_PER_THREAD / 300) * (940 + (160 * 9)) + 1024);
    }
    set_constants();
    opcode = (uint8_t*)mmap(NULL, 10000, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON | MAP_32BIT, -1, 0);
    fwrite(FIRST_100_LINES, 1, sizeof(FIRST_100_LINES), stdout);
    uint64_t line_number = 100, line_boundary = 1000;
    for (int thread = 0; thread < NUM_THREADS; thread++) pthread_create(&threads[thread], NULL, thread_func, (void*)(&thread_args[thread]));
    for (digits = 3; digits < MAX_DIGITS; digits++)
    {
        const int STRING_LEN = 940 + (160 * digits);
        for (int i = 0; i < 500; i++) shuffles[i] = _mm256_setzero_si256();
        string_ptr = string;
        uint8_t shuffle_init[] = { 8, 7, 6, 5, 4, 3, 2, 1, 0, '0', '0', '\n' }, * shuffle_init_ptr = shuffle_init + 11 - digits;
        for (int i = 100; i < 400; i += 10)
        {
            for (int j = i; j < i + 10; j++)
            {
                if (j % 3 == 0)
                {
                    if (j % 5 == 0)
                    {
                        memcpy(string_ptr, FizzBuzz, 9);
                        string_ptr += 9;
                    }
                    else
                    {
                        memcpy(string_ptr, Fizz, 5);
                        string_ptr += 5;
                    }
                }
                else if (j % 5 == 0)
                {
                    memcpy(string_ptr, Buzz, 5);
                    string_ptr += 5;
                }
                else
                {
                    memcpy(string_ptr, shuffle_init_ptr, digits + 1);
                    string_ptr += digits + 1;
                }
                if (shuffle_init[10] == '9') shuffle_init[10] = '0';
                else shuffle_init[10]++;
            }
            if (shuffle_init[9] == '9') shuffle_init[9] = '0';
            else shuffle_init[9]++;
        }
        bytecode_ptr = bytecode;
        shuffle_idx = 0;
        const int FIRST_BOUNDARY = 312 + (54 * digits), SECOND_BOUNDARY = 624 + (107 * digits), THIRD_BOUNDARY = 940 + (160 * digits);
        fill_shuffles(0, FIRST_BOUNDARY);
        fill_shuffles(FIRST_BOUNDARY, SECOND_BOUNDARY);
        fill_shuffles(SECOND_BOUNDARY, THIRD_BOUNDARY);
        CODE_SIZE = bytecode_ptr - bytecode;
        generate_opcode();
        opcode_exec = opcode;
        const int runs_to_buffer = (BUFFER_SIZE / STRING_LEN), runs_to_digit = (line_boundary - line_number) / 300;
        int runs_per_thread = (runs_to_digit < runs_to_buffer ? runs_to_digit : runs_to_buffer) / NUM_THREADS;
        int THREADS_TO_DO = NUM_THREADS;
        if (runs_per_thread == 0)
        {
            runs_per_thread = runs_to_digit;
            THREADS_TO_DO = 1;
        }
        int temp_line_number = line_number;
        for (int thread = 0; thread < THREADS_TO_DO; thread++)
        {
            thread_args[thread].start_number = temp_line_number;
            thread_args[thread].end_number = temp_line_number + (runs_per_thread * 300);
            thread_args[thread].runs = runs_per_thread;
            thread_args[thread].buffer_len = runs_per_thread * STRING_LEN;
            temp_line_number += runs_per_thread * 300;
        }
        for (int thread = 0; thread < THREADS_TO_DO; thread++) pthread_spin_unlock(&thread_args[thread].work);
        while(line_number < line_boundary)
        {
            for (int thread = 0; thread < THREADS_TO_DO; thread++) 
            {
                if (thread_args[thread].start_number >= line_boundary) continue;
                pthread_mutex_lock(&thread_args[thread].idle);
                line_number = thread_args[thread].end_number;
                fwrite(thread_args[thread].thread_buffer, 1, thread_args[thread].buffer_len, stdout);
                thread_args[thread].start_number += (runs_per_thread * THREADS_TO_DO * 300);
                if (thread_args[thread].start_number >= line_boundary) continue;
                thread_args[thread].end_number = thread_args[thread].start_number + (runs_per_thread * 300);
                if (thread_args[thread].end_number > line_boundary) thread_args[thread].end_number = line_boundary;
                thread_args[thread].runs = (thread_args[thread].end_number - thread_args[thread].start_number) / 300;
                thread_args[thread].buffer_len = thread_args[thread].runs * STRING_LEN;
                pthread_spin_unlock(&thread_args[thread].work);
            }
        }
        line_boundary *= 10;
    }
    fprintf(stdout, "1000000000\n");
    return 0;
}
```
