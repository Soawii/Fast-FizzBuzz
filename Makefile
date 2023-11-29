FizzBuzz: FizzBuzz.c
	gcc FizzBuzz.c -o FizzBuzz -mavx2 -no-pie

test: FizzBuzz
	./FizzBuzz > /dev/null	
