#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>

__m256i number, shuffle, ascii_number;
__m256i VEC_198, ONE, VEC_246;

__m256i shuffles[5000]; 
uint8_t shuffle_idx = 0;

const char Fizz[] = "Fizz\n", Buzz[] = "Buzz\n", FizzBuzz[] = "FizzBuzz\n";

int digits;

int8_t code[20000], * code_ptr = code;

int CODE_SIZE;

#define PAGE_SIZE 4096
#define BUFFER_SIZE (1 << 20)

alignas(PAGE_SIZE) char buffer1[BUFFER_SIZE + 2048], buffer2[BUFFER_SIZE + 2048], * current_buffer = buffer1, * buffer_ptr = buffer1;
int buffer_in_use = 0;

char string[20000], * string_ptr;

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

const char FIRST_100_LINES[] = "1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz\n16\n17\nFizz\n19\nBuzz\nFizz\n22\n23\nFizz\nBuzz\n26\nFizz\n28\n29\nFizzBuzz\n31\n32\nFizz\n34\nBuzz\nFizz\n37\n38\nFizz\nBuzz\n41\nFizz\n43\n44\nFizzBuzz\n46\n47\nFizz\n49\nBuzz\nFizz\n52\n53\nFizz\nBuzz\n56\nFizz\n58\n59\nFizzBuzz\n61\n62\nFizz\n64\nBuzz\nFizz\n67\n68\nFizz\nBuzz\n71\nFizz\n73\n74\nFizzBuzz\n76\n77\nFizz\n79\nBuzz\nFizz\n82\n83\nFizz\nBuzz\n86\nFizz\n88\n89\nFizzBuzz\n91\n92\nFizz\n94\nBuzz\nFizz\n97\n98\nFizz\n";

int main()
{
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
       ascii_number = _mm256_sub_epi8(number, VEC_198);
       uint64_t RUNS, RUNS_TO_DIGIT = (line_boundary - line_number) / 300, RUNS_TO_BUFFER;
       while (1)
       {
           RUNS_TO_BUFFER = ((current_buffer + BUFFER_SIZE) - buffer_ptr) / THIRD_BOUNDARY + 1;
           RUNS = RUNS_TO_BUFFER < RUNS_TO_DIGIT ? RUNS_TO_BUFFER : RUNS_TO_DIGIT;
           if (RUNS == 0) break;
           for (int i = 0; i < RUNS; i++) interpret_bytecode();
           if (buffer_ptr >= current_buffer + BUFFER_SIZE)
           {
               fwrite(current_buffer, 1, buffer_ptr - current_buffer, stdout);
               buffer_ptr = current_buffer;
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
