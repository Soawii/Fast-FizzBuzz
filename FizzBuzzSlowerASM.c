#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>
#include <fcntl.h>

__m256i number, shuffle_vec, VEC_198, ONE, VEC_246, shuffle_number;
static __m256i shuffles[50];
uint8_t shuffle_idx = 0;

const char Fizz[] = "Fizz\n", Buzz[] = "Buzz\n", FizzBuzz[] = "FizzBuzz\n";

int digits;

uint8_t* jit, * jit_ptr;
typedef uint8_t* (*jit_fn)(uint8_t*, int);

int8_t code[200], *code_ptr = code;

int CODE_SIZE;

#define PAGE_SIZE 4096
#define BUFFER_SIZE (1 << 20)

alignas(PAGE_SIZE) char buffer1[BUFFER_SIZE + 1024], buffer2[BUFFER_SIZE + 1024], *current_buffer = buffer1, *buffer_ptr = buffer1;
int buffer_in_use = 0;

char string[2000], *string_ptr;

void* alloc_executable_memory(size_t size) 
{
    void* ptr = mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_PRIVATE | MAP_ANON | MAP_32BIT, -1, 0);
    return ptr;
}

void set_constants()
{
	memset(code, 0, sizeof(code));
    ONE = _mm256_set_epi64x(0, 1, 0, 1);
	VEC_246 = _mm256_set1_epi8(246);
	VEC_198 = _mm256_set_epi8(198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198, 198, 198, 198, 198, 198, 198, 198, 190, 191, 192, 193, 194, 195, 196, 197, 198);
}

void slow_compile()
{
    for (int i = 0; i < CODE_SIZE; i++)
    {
        int8_t c = code[i];
        if (c == 1)
        {
            shuffle_vec = _mm256_shuffle_epi8(shuffle_number, shuffles[shuffle_idx]);
            shuffle_vec = _mm256_sub_epi8(shuffle_vec, shuffles[shuffle_idx]);
            memcpy(buffer_ptr, &shuffle_vec, 32);
            buffer_ptr += 32;
            shuffle_idx++;
        }
        else if (c == 2)
        {
            number = _mm256_add_epi64(number, ONE);
            number = _mm256_max_epu8(number, VEC_246);
            shuffle_number = _mm256_sub_epi8(number, VEC_198);
        }
        else
        {
            buffer_ptr -= c;
        }
    }
    shuffle_idx = 0;
}

#define get_rip_distance(ptr) {uint32_t rip_distance = (uint8_t*)(ptr) - (uint8_t*)(jit_ptr + 4); memcpy(jit_ptr, &rip_distance, 4); jit_ptr += 4;}

void fast_compile()
{
    jit_ptr = jit;
    uint32_t offset = 0, rip_distance = 0;
    __m256i *shuffles_ptr = shuffles;
    *jit_ptr++ = 0xC5; *jit_ptr++ = 0x7D; *jit_ptr++ = 0x6F; *jit_ptr++ = 0x0D; get_rip_distance(&number); // vmovdqa ymm9, YMMWORD PTR [rip + number]
    *jit_ptr++ = 0xC4; *jit_ptr++ = 0x41; *jit_ptr++ = 0x35; *jit_ptr++ = 0xF8; *jit_ptr++ = 0xEB; // vpsubb ymm13, ymm9, ymm11
    uint8_t* start = jit_ptr;
    for (int i = 0; i < CODE_SIZE; i++)
    {
        int8_t c = code[i];
        if (c == 1)
        {
            *jit_ptr++ = 0xC5; *jit_ptr++ = 0x7D; *jit_ptr++ = 0x6F; *jit_ptr++ = 0x35; get_rip_distance(shuffles_ptr);                                              // | = vmovdqa ymm14, YMMWORD PTR [shuffles_ptr] (or vmovdqa ymm14, YMMWORD PTR [rip + ((uint8_t*)shuffles_ptr - (jit_ptr + 4))])
            *jit_ptr++ = 0xC4; *jit_ptr++ = 0x42; *jit_ptr++ = 0x15; *jit_ptr++ = 0x00; *jit_ptr++ = 0xFE;   // vpshufb ymm15, ymm13, ymm14
            *jit_ptr++ = 0xC4; *jit_ptr++ = 0x41; *jit_ptr++ = 0x05; *jit_ptr++ = 0xF8; *jit_ptr++ = 0xFE;   // vpsubb ymm15, ymm15, ymm14
            *jit_ptr++ = 0xC5; *jit_ptr++ = 0x7E; *jit_ptr++ = 0x7F; *jit_ptr++ = 0xBF; memcpy(jit_ptr, &offset, 4); jit_ptr += 4;  // vmovdqu YMMWORD PTR [rdi + offset], ymm15
            offset += 32;
            shuffles_ptr++;
        }
        else if (c == 2)
        {
            *jit_ptr++ = 0xC4; *jit_ptr++ = 0x41; *jit_ptr++ = 0x35; *jit_ptr++ = 0xD4; *jit_ptr++ = 0xCA; // vpaddq ymm9, ymm9, ymm10
            *jit_ptr++ = 0xC4; *jit_ptr++ = 0x41; *jit_ptr++ = 0x35; *jit_ptr++ = 0xDE; *jit_ptr++ = 0xCC; // vpmaxub ymm9, ymm9, ymm12
            *jit_ptr++ = 0xC4; *jit_ptr++ = 0x41; *jit_ptr++ = 0x35; *jit_ptr++ = 0xF8; *jit_ptr++ = 0xEB; // vpsubb ymm13, ymm9, ymm11
        }
        else offset += c;
    }
    *jit_ptr++ = 0x48; *jit_ptr++ = 0x81; *jit_ptr++ = 0xC7; memcpy(jit_ptr, &offset, 4); jit_ptr += 4; // add rdi, offset
    *jit_ptr++ = 0xFF; *jit_ptr++ = 0xCE; // dec esi
    *jit_ptr++ = 0x0F; *jit_ptr++ = 0x85; get_rip_distance(start); // jnz start
    *jit_ptr++ = 0xC5; *jit_ptr++ = 0x7D; *jit_ptr++ = 0x7F; *jit_ptr++ = 0x0D; get_rip_distance(&number); // vmovdqa YMMWORD PTR [rip + number], ymm9
    *jit_ptr++ = 0x48; *jit_ptr++ = 0x89; *jit_ptr++ = 0xF8; // mov rax, rdi
    *jit_ptr++ = 0xC3; // ret
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
    jit = (uint8_t*)alloc_executable_memory(16000);
    memcpy(buffer_ptr, FIRST_TEN, strlen(FIRST_TEN));
    buffer_ptr += strlen(FIRST_TEN);
	for (digits = 2; digits < 10; digits++)
	{
	    for (int i = 0; i < 50; i++) shuffles[i] = _mm256_set1_epi8(0);
	    string_ptr = string;
	    uint8_t shuffle_init[] = {9, 8, 7, 6, 5, 4, 3, 2, 1, 0, '0', '\n'}, *shuffle_init_ptr = shuffle_init + 11 - digits;
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
	    shuffle_number = _mm256_sub_epi8(number, VEC_198);
	    fast_compile();
	    jit_fn func = jit;
        uint64_t RUNS, RUNS_TO_DIGIT, RUNS_TO_BUFFER;
        while(1)
        {
            RUNS_TO_DIGIT = (line_boundary - line_number) / 30; 
            RUNS_TO_BUFFER = ((current_buffer + BUFFER_SIZE) - buffer_ptr) / THIRD_BOUNDARY + 1;
            RUNS = RUNS_TO_BUFFER < RUNS_TO_DIGIT ? RUNS_TO_BUFFER : RUNS_TO_DIGIT;
            if (RUNS == 0) break;
            asm ("vmovdqa %0, %%ymm10\n\t"
            "vmovdqa %1, %%ymm11\n\t"
            "vmovdqa %2, %%ymm12\n\t"
            :
            : "" (ONE),
            "" (VEC_198),
            "" (VEC_246));
            buffer_ptr = func(buffer_ptr, RUNS);
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
            line_number += RUNS * 30;
        }
        line_boundary *= 10;
	}
	fwrite(current_buffer, 1, buffer_ptr - current_buffer, stdout);
	printf("1000000000\n");
	return 0;
}
