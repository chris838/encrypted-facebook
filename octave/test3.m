for qf=80:90

z = 10000;
e=0;

for i=1:z

# Create some random data 
d = uint8( round(256*rand(8,8)) );

# Perform JPEG compression
nothing =imread('test.jpg');
imwrite(d, 'test.jpg', 'Quality', qf);
d2 = imread('test.jpg');

# Calculate bit errors
[e1 e2] = biterr( d, d2 );

e = e + (e2/z);

end

e

end