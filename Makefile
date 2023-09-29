FizzBuzz: FizzBuzz.c
	gcc FizzBuzz.c -o FizzBuzz -O3 -march=native

test: FizzBuzz
	time ./FizzBuzz | pv > /dev/null	
