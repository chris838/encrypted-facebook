a = [-3 2 1];
A = 	[-3 0 0 0 0;
	  2 -3 0 0 0;
	  1 2 -3 0 0;
	  0 1 2 -3 0;
	  0 0 1 2 -3;
	  0 0 0 1  2;
	  0 0 0 0  1];
b = [1 0 0 2 2];

# Convolve the two
r = A * b'

# Reverse using matrix operations
r / b'
r \ A
