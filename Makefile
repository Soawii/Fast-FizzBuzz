FizzBuzz: FizzBuzz.c
	gcc FizzBuzz.c -o FizzBuzz -O2 -mavx2 -no-pie

test: FizzBuzz
	./FizzBuzz | pv > /dev/null	
