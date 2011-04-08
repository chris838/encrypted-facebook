a = 	[-3 0 0 0 0;
	  2 -3 0 0 0;
	  1 2 -3 0 0;
	  0 1 2 -3 0;
	  0 0 1 2 -3;
	  0 0 0 1  2;
	  0 0 0 0  1];
b = [1 0 0 2 2];

# Multiply the two 
c = b * a'

# Reverse using matrix operations
c / a'
