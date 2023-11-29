FizzBuzz: FizzBuzz.c
	gcc FizzBuzz.c -o FizzBuzz -mavx2 -no-pie -march=native

test: FizzBuzz
	./FizzBuzz > /dev/null	
