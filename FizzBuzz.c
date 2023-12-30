#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <inttypes.h>
#include <immintrin.h>
#include <stdalign.h>
#include <pthread.h>
 
#define LINES_PER_THREAD 450000 // has to be a multiple of 300, should be changed around to fit your PC
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
