function w=computeWeight( val )
w = val;
w = bitand(bitshift(w, -1), uint32(1431655765)) + ...
    bitand(w, uint32(1431655765));

w = bitand(bitshift(w, -2), uint32(858993459)) + ...
    bitand(w, uint32(858993459));

w = bitand(bitshift(w, -4), uint32(252645135)) + ...
    bitand(w, uint32(252645135));

w = bitand(bitshift(w, -8), uint32(16711935)) + ...
    bitand(w, uint32(16711935));

w = bitand(bitshift(w, -16), uint32(65535)) + ...
    bitand(w, uint32(65535));
end