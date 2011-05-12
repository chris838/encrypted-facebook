input metauml;
beginfig(1);

    Class.I("IFec")()
    (
        "codeLength(data_length: int): int",
        "dataLength(code_length: int): int",
        "encode(data: vector<byte> ref)",
        "decode(data: vector<byte> ref)"
    );
    classStereotypes.I("<<interface>>");
    
    Class.S("SchifraFec: {abstract}")()();
    ClassTemplate.T("N: int", "M: int")(S);
    
    Class.RSa("ReedSolomon15Fec")()();
    
    Class.RSb("ReedSolomon255Fec")()();
    
    I.nw = (0,20);
    
    leftToRight(50)(S,I);
    topToBottom(30)(S,RSa,RSb);
    
    drawObjects(I,S,T,RSa,RSb);
    
    link(realization)
        (pathStepY(RSa.e, I.s, 0));
    link(realization)
        (pathStepY(RSb.e, I.s, 0));
        
    link(inheritance)
        (pathStepX(RSa.w, S.w, -20));
    link(inheritance)
        (pathStepX(RSb.w, S.w, -20));
    
    item(iAssoc)("<<bind>>")(obj.w =.8[RSa.nw,S.sw]);
    item(iAssoc)("<N::15, M::9>")(obj.w =.2[RSa.nw,S.sw]);        
    item(iAssoc)("<N::255, M::223>")(obj.w =.2[RSb.nw,RSa.sw]);
    
endfig;
end