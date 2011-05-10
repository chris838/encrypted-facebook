input metauml;
beginfig(1);

    Class.Il("IeFBLib")()();
    classStereotypes.Il("<<interface>>");
    
    Class.A("BasicLibary")
    (
            "-factory: ILibFactory ref",
            "-crypto : ICrypto ref",
            "-fec: IFec ref",
            "-string_codec: IStringCodec ref"    
    )
    ();
    
    Class.Ilf("IeFBLibFactory")()
    (
        "+create_IConduitImage(): IConduitImage ref",
        "+create_ICrypto():ICrypto ref",
        "+create_IFec(): IFec ref",
        "+create_IStringCodec(): IStringCodec ref"
    );
    classStereotypes.Ilf("<<interface>>");
    
    Class.Fa("Haar20KiBFactory")()();
    Class.Fb("Scaled165KiBFactory")()();
    
    Class.Icy("ICrypto")()();
    classStereotypes.Icy("<<interface>>");

    Class.Ic("IConduitImage")()();
    classStereotypes.Ic("<<interface>>");
    
    Class.If("IFec")()();
    classStereotypes.If("<<interface>>");
    
    Class.Is("IStringCodec")()();
    classStereotypes.Is("<<interface>>");
    
    Class.E("HaarWTConduitImage")()();
    
    leftToRight(30)(Il,A);
    topToBottom(25)(Il,Ilf);
    
    leftToRight(80)(Ilf,Icy);
    topToBottom(15)(Icy,Ic,If,Is);
    
    Fa.nw = Ilf.sw - (0,50);
    Fb.nw = Fa.sw - (0,15);
    
    leftToRight(40)(Fa,E);
    
    drawObjects(E);
    
    draw (E.nw + (-35,28))--(E.ne + (10,28))--(E.se + (10,-10))--(E.sw + (-35,-10))--cycle dashed evenly;    
    fill (E.nw + (-35,28))--(E.ne + (10,28))--(E.se + (10,-10))--(E.sw + (-35,-10))--cycle withcolor (.9,1,.9);
    label.bot( btex Example concrete sub-class etex, E.s - (0,10));
    
    drawObjects(Il,A,Ilf,Fa,Fb, Icy,Ic,If,Is, E);
    
    link(realization)(pathStepX(Fa.w, Ilf.w, -20));
    link(realization)(pathStepX(Fb.w, Ilf.w, -20));
    link(realization)(pathStepY(A.n, Il.n, 10));
    
    link(realization)(pathStepX(E.n, Ic.w, 0));

    link(composition)(pathStepX(Ilf.n, A.w, 40));
        
    link(composition)(pathStepX(Icy.e, A.e, 20));
    link(associationUni)(pathStepX(Ic.e, A.e + (0,20), 20));
    link(composition)(pathStepX(If.e, A.e, 20));
    link(composition)(pathStepX(Is.e, A.e, 14));

    item(iAssoc)("<<creates>>")(obj.s =.5[Fa.n,E.n] + (0,10));
    link(association)(pathStepY(Fa.n, E.n - (20,0), 20));
    

    
endfig;
end