FizzBuzz: FizzBuzz.c
	gcc FizzBuzz.c -o FizzBuzz -O3 -march=native

test: FizzBuzz
	./FizzBuzz | pv > /dev/null	
