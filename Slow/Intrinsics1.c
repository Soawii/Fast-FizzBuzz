#include <stdio.h>
#include <immintrin.h>
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
                fwrite(current_buffer, 1, buffer_ptr - current_buffer, stdout);
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
