#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char* numberPtrs[16];

const int table[] = { 4, 7, 2, 11, 2, 7, 12, 2, 12, 7, 2, 11, 2, 7, 12, 2 }; 

int main()
{
	char output[15000]; 
	char output_string[500];
	char* output_ptr = output, * end_ptr = output + sizeof(output), * string_ptr = output_string;
	printf("1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\n");
	long long int base_number = 1, current_number = 40;
	for (int Digits = 1; Digits < 9; Digits++)
	{
		int block_size = sprintf(string_ptr, "Buzz\n%lli1\nFizz\n%lli3\n%lli4\nFizzBuzz\n%lli6\n%lli7\nFizz\n%lli9\nBuzz\nFllizz\n%lli2\n%lli3\nFizz\nBuzz\n%lli6\nFizz\n%lli8\n%lli9\nFizzBuzz\n%lli1\n%lli2\nFizz\n%lli4\nBuzz\nFizz\n%lli7\n%lli8\nFizz\n", base_number, base_number, base_number, base_number, base_number, base_number, base_number + 1, base_number + 1, base_number + 1, base_number + 1, base_number + 1, base_number + 2, base_number + 2, base_number + 2, base_number + 2, base_number + 2);
		memcpy(output_ptr, string_ptr, block_size);
		output_ptr += block_size;
		base_number *= 10;
		char* temp = string_ptr;
		for (int i = 0; i < 16; i++)
		{
			temp += table[i] + Digits;
			numberPtrs[i] = temp;
		}
		int runs = 1;
		while (runs > 0)
		{
			int runs_to_fill_output = (int)((end_ptr - output_ptr) / block_size), runs_to_fill_digit = (((base_number * 10) - current_number) / 30);
			runs = runs_to_fill_output > runs_to_fill_digit ? runs_to_fill_digit : runs_to_fill_output;
			current_number += runs * 30;
			for (int i = 0; i < runs; i++)
			{
				for (int j = 0; j < 16; j++)
				{
					char* temp = numberPtrs[j];
					if (*temp < '7') *temp += 3;
					else
					{
						*temp-- -= 7;
						while (*temp == '9') *temp-- = '0';
						*temp += 1;
					}
				}
				memcpy(output_ptr, string_ptr, block_size);
				output_ptr += block_size;
			}
			if (runs > 0)
			{
				fwrite(output, 1, sizeof(output) - (end_ptr - output_ptr), stdout);
				output_ptr = output;
			}
		}
	}
	return 0;
}
