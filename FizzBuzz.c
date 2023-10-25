#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>
#include <fcntl.h>

#define BUFFER_SIZE (1 << 20)

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

#define PAGE_SIZE 4096

alignas(PAGE_SIZE) char buffer1[BUFFER_SIZE + 1024], buffer2[BUFFER_SIZE + 1024], * current_buffer = buffer1, * buffer_ptr = buffer1;
int buffer_in_use = 0;

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
    fcntl(1, F_SETPIPE_SZ, BUFFER_SIZE);
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
            RUNS_TO_BUFFER = ((current_buffer + BUFFER_SIZE) - buffer_ptr) / THIRD_BOUNDARY + 1;
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
