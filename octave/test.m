eee = [;];
order = 4;
factor = (255 / (2**order-1)) + 1; 
offset =  ((factor * (2**order-1)) - 255) / 2;
qqq = [;];
for k = 0:1:1000;
	ee = [];
	qq = [];
	r = round( (2**order -1)*rand(8,8) );
	for q = 84:1:86;
    		i = uint8( factor*r - offset );
    		junk1 = imread("junk.jpg");
    		imwrite(i, 'jpegcode.jpg', 'Quality', q);
    		j = imread('jpegcode.jpg');
    		r2 = uint8( (j + offset) / factor );
    		qq = [qq q];
    		rg = bitxor(r,bitshift(r,-1));
    		r2g = bitxor(r,bitshift(r2,-1));
    		ee = [ee length(find(bitxor(rg,r2g)))];
	end    		
	qqq = [qqq; qq];
	eee = [eee; ee];
end
plot(mean(qqq), mean(eee) / 192 );
xlabel('quality');
ylabel('bit errors');