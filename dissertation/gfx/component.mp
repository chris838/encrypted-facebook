input metauml;
beginfig(1);

    Component.A("efb")();
    Component.B("pagecept")();
    Component.F("faceapi")();
    

    Group.C(A, B, F);


    Component.T("Toolbar XUL")();
    
    Component.M("C++ Module")();
    
    
    topToBottom(60)(B, A, F);
    leftToRight(60)(B, T);
    leftToRight(60)(F, M);
    
    
    drawObject(C);
    drawObject(T);
    drawObject(M);


    item.aName(iAssoc)("control handlers")(aName.se = A.n);
    item.aa(iAssoc)("Decryption and UI")(aa.se = aName.ne);
    item.bName(iAssoc)("handlers")(bName.sw = A.ne);
    item.bb(iAssoc)("Toolbar control")(bb.sw = bName.nw);
    item.cName(iAssoc)("interface")(cName.se = F.n);
    item.cc(iAssoc)("Facebook platform")(cc.se = cName.ne);
    item.dName(iAssoc)("interface")(dName.sw = M.n);
    item.dd(iAssoc)("Crypto & codec")(dd.sw = dName.nw);

    
    Component.G("JavaScript Module")(C, aa, bb, cc, aName, bName, cName);
    drawObject(G);
    
    Note.N("Stego! library", "interface");
    N.n = (A.w - (50,10));
    drawObject(N);
    


    link.(providedInterface)( A.n -- (A.n + (0,40)) );
    link.(requiredInterface)( B.s -- (A.n + (0,40)) );
    
    link.(providedInterface)( F.n -- (A.s - (0,20)) );
    link.(requiredInterface)( A.s -- (A.s - (0,20)) );
    
    link(requiredInterface)( A.se -- (A.se - (0,20)) );
    
    link(providedInterface)( A.ne -- (A.ne + (0,40)) );
    
    link(requiredInterface)( A.w -- (A.w - (15,0)) );
    link(dashedLink)( N.n -- (A.w - (20,0)) );
    
Class.X("")()();
X.info.iAttributeStack.top := 0;
X.info.iAttributeStack.bottom := 0;
X.info.iMethodStack.top := 0;
X.info.iMethodStack.bottom := 0;
X.c = .5[(A.se - (0,30)), (M.n +  (0,30))];
drawObject(X);

drawRelation(dependency)( X.w -- (A.se - (-10,30)) );



Class.Y("")()();
Y.info.iAttributeStack.top := 0;
Y.info.iAttributeStack.bottom := 0;
Y.info.iMethodStack.top := 0;
Y.info.iMethodStack.bottom := 0;
Y.c = .5[(A.ne + (0,40)), (T.s -  (0,20))];
drawObject(Y);

drawRelation(dependency)( (A.ne + (0,40)) -- Y.w );


    link(providedInterface)( M.n --  .5[M.n, X.e] );
    link(requiredInterface)( X.e --  .5[M.n, X.e] );
    
    link(requiredInterface)( T.s --  .5[T.s, Y.e] );
    link(providedInterface)( Y.e --  .5[T.s, Y.e] );

    
endfig;
end