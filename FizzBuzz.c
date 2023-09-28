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
