input metauml;
beginfig(1);

    Class.I("ICrypto")()
    (
        "calculateHeaderSize(numOfIds: int): int",
        "retrieveHeaderSize(data: vector<byte> ref): int",
        "decryptMessage(data: vector<byte> ref)",
        "generateKeys( passphrase: string ref, ...)",
        "loadKeys(passphrase: string ref, ...)",
        "loadIdKeyPair(id: FacebookId ref, ...)",
        "setUserId(id: FacebookId ref)"
    );
    classStereotypes.I("<<interface>>");
    
    Class.B("BotanRSACrypto")()();
    ClassTemplate.T("N: int", "M: int")(B);
    
    I.nw = (0,20);
    
    topToBottom(40)(I,B);
    
    drawObjects(I,B,T);
    
    link(realization)
        (pathStepY(B.n, I.s, 0));
        
    
endfig;
end