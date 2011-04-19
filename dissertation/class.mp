input metauml;
beginfig(1);

    Class.I("IConduitImage")()();

    Class.A("BufferedConduitImage")()();
    
    Class.B("HaarWTConduitImage")()();
    
    I.nw = (20,20);
    
    topToBottom(45)(I,A);
    
    B.ne = A.sw + (20,-30);

    drawObjects(I,A,B);
    
    clink(inheritance)(I, A);
    clink(inheritance)(A, B);

    
endfig;
end