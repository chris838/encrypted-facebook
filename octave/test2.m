# Generate an initial bitmask from the quantisation matrix

# First read in a sample Facebook image
sam = jpeg_read('sample.jpg');

# Get the quant matrix for the luminance
q = sam.quant_tables{sam.comp_info(1).quant_tbl_no};

# Convert to bit mask
m = uint16( ceil( log2(q) ) );
m = 2.^(m+1);
m = bitor( m, 2*m );
format hex;
m = bitand(m , ones(8)*0x00ff );
m(1,1) = 0;
m

# Load an 8x8 greyscale template jpeg and set its quant values
i =  jpeg_read('template.jpg');
i.quant_tables{1} = q;
# Also set its coefficients to zero
i.coef_arrays{1} = zeros(8);

# Create some random data and mask off unwanted bits
d = uint16( round(2047*rand(8,8)) );
d= bitand( d , m);
# Now format the data into double precision (unsigned)
d = double(d) ;

off = 50;

# Save data to the coefficients
i.coef_arrays{1} = quantize(d-off,q);

# Perform JPEG compression
jpeg_write(i, 'test.jpg');
temp = imread("test.jpg");
imwrite(temp, 'test2.jpg', 'Quality', 85);
j = jpeg_read('test2.jpg');

# Retrieve data
d2 = dequantize( j.coef_arrays{1} , q) + off;
#d2 =  bitand( d2 , m);

format;
i.coef_arrays{1}
max( max(temp) )
j.coef_arrays{1}
d-d2

# Calculate bit errors
#e = bitxor( uint16(d) , d2 );
#e

# Caclulate capacity