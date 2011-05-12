# Create some random data 
d = uint8( round(256*rand(8,8)) );

# Perform JPEG compression
nothing =imread('test.jpg');
imwrite(d, 'test.jpg', 'Quality', 85);
d2 = imread('test.jpg');

# Calculate bit errors
[e1 e2] = biterr( d, d2 );

e2
