input metauml;
beginfig(1);

    Class.CImg("cimg_library::CImg")()();
    ClassTemplate.T("T")(CImg);

    Class.I("IConduitImage")()
    (
        "+getMaxData(): int",
        "+implantData( data: vector<byte> ref )",
        "+extractData( data: vector<byte> ref )"
    );
    classStereotypes.I("<<interface>>");

    Class.A("BufferedConduitImage : {abstract}")
    (
        "-rbuffer: deque<byte>",
        "-rhead: int",
        "-wbuffer: deque<byte>",
        "-whead: int"
    )
    (
        "-read(): byte",
        "-write(b: byte)",
        "-flush()"
    );
    
    Class.B("HaarWTConduitImage")()
    (
        "-getBlockCoords( i: int ref, j: int ref, idx: int)",
        "-encodeInBlock(data: vector<byte>, i: int, j: int)"
    );
    

    Class.C("ScaledConduitImage : {abstract}")
    (
        "-order: int"
    )
    (
        "#encodeInPixel(b: byte, i: int, j: int)",
        "#decodeFromPixel(i: int, j:int): byte"
    );

    
    Class.Ca("Scaled3ConduitImage")()
    (
        "-getBlockCoords(i: int ref, j: int ref, idx: int)",
        "-encodeInBlock(data: vector<byte>, i: int, j: int)"
    );
    
    Class.Cb("Scaled4ConduitImage")()
    (
        "-getBlockCoords(i: int ref, j: int ref, idx: int)",
        "-encodeInBlock(data: vector<byte>, i: int, j: int)"
    );
    
    I.nw = (0,20);
    
    topToBottom(40)(CImg, A,C);
    
    leftToRight(15)(C,Ca);
    topToBottom(20)(I,B,Ca,Cb);

    

    drawObjects(CImg,I,A,T,B,C,Ca,Cb);
    
    link(realization)
        (pathStepX(B.e, I.e, 20));

    link(realization)
        (pathStepX(Ca.e, I.e, 20));
        
    link(realization)
        (pathStepX(Cb.e, I.e, 20));
    
    link(inheritance)
        (pathStepY(A.n, CImg.s, 0));
    
    item(iAssoc)("<<bind>>")(obj.w =.5[A.n,CImg.s]);
    item(iAssoc)("<T::byte>")(obj.w =.2[A.n,CImg.s]);
        
    link(inheritance)
        (pathStepY(B.s, A.s, -20));
        
    
    link(inheritance)
        (pathStepY(C.n, A.s, 0));
        
    link(inheritance)
        (pathStepY(Ca.s, C.s, 0));

    link(inheritance)
        (pathStepY(Cb.w, C.s, 0));
    
endfig;
end