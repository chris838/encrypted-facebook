eee = [;];
qqq = [;];
for k = 0:1:1000;
	ee = [];
	qq = [];
	r = round(15*rand(8,8));
	for q = 79:1:87;
    		i = uint8( 18*r );
    		junk1 = imread("junk.jpg");
    		imwrite(i, 'jpegcode.jpg', 'Quality', q);
    		j = imread('jpegcode.jpg');
    		r2 = uint8( j/18 );
    		qq = [qq q];
    		rg = bitxor(r,bitshift(r,-1));
    		r2g = bitxor(r,bitshift(r2,-1));
    		ee = [ee length(find(bitxor(rg,r2g)))];
	end    		
	qqq = [qqq; qq];
	eee = [eee; ee];
end
plot(mean(qqq), max(eee));
xlabel('quality');
ylabel('bit errors');