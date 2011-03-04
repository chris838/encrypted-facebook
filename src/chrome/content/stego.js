/**
    Text steganography functions.
*/
stego = {
    //! Retrieve word given index in list of words of that length
    retrieveWord : function(length, index) {
        if ((length >= minw) && (length <= maxw) &&
            (index >= 0) && (index < nwords[length])) {
            return cwords[length].substring(length * index, length * (index + 1));
        }
        return "";
    },
    
    //! Obtain word by index in complete dictionary
    indexWord : function(index) {
        if ((index >= 0) && (index < twords)) {
            var j;
    
            for (j = minw; j <= maxw; j++) {
                if (index < nwords[j]) {
                    break;
                }
                index -= nwords[j];
            }
            return stego.retrieveWord(j, index);
        }
        return "";
    },
    
    /**
        Convert an array of bytes (integers between 0 and 255) to a hexadecimal string.
    */
    byteArrayToHex : function(byteArray) {
        var result = "", i;
        
        for (i = 0; i < byteArray.length; i++) {
          result += ((byteArray[i] < 16) ? "0" : "") +
                    byteArray[i].toString(16);
        }
        return result;
    },
    
    /**
        Parse a sequence of hexadecimal digits into an array of bytes (integers between 0 and 255).
    */
    hexToByteArray : function(hexString) {
        var byteArray = new Array(), i;
    
        if (hexString.length % 2) {
            byteArray += "0";
        }
        if ((hexString.indexOf("0x") == 0) ||
            (hexString.indexOf("0X") == 0)) {
            hexString = hexString.substring(2);
        }
        
        for (i = 0; i < hexString.length; i += 2) {
            byteArray[Math.floor(i / 2)] =
                parseInt(hexString.slice(i, i + 2), 16);
        }
        return byteArray;
    },
    /*
        Convert an array of bytes to a base64 string.
    */
    byteArrayToBase64 : function(b){
        var base64code = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        var b64String = "";
        var c1,c2,c3,c4;
        // Cycle through bytes in groups of three, outputting four characters for each.
        for (i=0; i<b.length; i+=3)
        {
            // If we don't have a multiple of three bytes then return empty string (failure).
            if (i >= b.length) return "";
            // Otherwise work out the correct four characters
            c1 = c2 = c3 = c4 = 0;
            //
            c1 = b[i] >> 2; 
            c2 = ((b[i] & 0x03) << 4) | (b[i+1] >> 4); 
            c3 = ((b[i+1] & 0x0f) << 2) | (b[i+2] >> 6); 
            c4 = b[i+2] & 0x3f;
            b64String += base64code.charAt( c1 );
            b64String += base64code.charAt( c2 );
            b64String += base64code.charAt( c3 );
            b64String += base64code.charAt( c4 );
            // If we have reached 64 characters (48 bytes) then output an end line
            if ( (i+3)%48 == 0 ) b64String += '\n';
        }
        return b64String;
    },
    
    /**
        Parse a sequence of base64 digits into an array of bytes. We assume groups of four digits, possibly interspersed by newlines but with no other characters. On failure returns an empty array.
    */
    base64ToByteArray : function(s) {
        var base64code = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        var byteArray = new Array();
        var i,j,b1,b2,b3, t = new Array(0x00,0x00,0x00,0x00);
        // Cycle through characters, ignoring newlines
        for (i=0; i<s.length;)
        {
            if ( s.charAt(i) != '\n' )
            {
                b1 = b2 = b3 = 0x00;
                // The next four characters...
                for (j=0;j<4;j++) {
                    // .. first we check for four valid base64 digits...
                    if (i+j >= s.length || base64code.indexOf( s.charAt(i+j) ) == -1) {
                        // Invalid base64 sequence, return empty array
                        window.alert( s.charAt(i+j) );
                        return new Array();
                    }
                    t[j] = base64code.indexOf( s.charAt(i+j) ) & 0x3f;
                }
                // ...if OK, we create three bytes to add.
                b1 = (t[0] << 2) | (t[1] >> 4);
                b2 = ((t[1] & 0x0f) << 4) | (t[2] >> 2);
                b3 = ((t[2] & 0x03) << 6) | t[3];
                // Add bytes to the byte array
                byteArray.push(b1);
                byteArray.push(b2);
                byteArray.push(b3);
                i+= 4;
            }
            else i++;
        }
        return byteArray;
    },
    
    //! Hide text as words
    hideText : function(msg, b64) {
        
        //! Sanity check dictionary length.
        if (twords != (1 << 16)) {
            window.alert("Error!  Dictionary (stegodict.js) contains " +
                  twords + " words; " + (1 << 16) + " words required.");
        }
            
        var ct = new Array(), padded = false;
        var purng = new stego.LEcuyer((new Date()).getTime());
        
        // We assume msg is a hex string. We now encode it into a byte array.
        if (b64)    ct = stego.base64ToByteArray(msg);
        else        ct = stego.hexToByteArray(msg);
        
        //  Cipher text should always be an even number of bytes. If it isn't, pad it with a zero and set a flag to indicate we've added a pad.
        if (ct.length & 1) {
            ct[ct.length] = 0;
            padded = true;
        }
        
        //  Walk through cipher text two bytes at a time, assembling each pair into an index into our table of words. Append each word to the hidden text. 
        var i, w, l = "", t = "";
        var maxLine = 72000
            sl = 0, sc = 0, fpar = false,
            parl = purng.nextInt(9) + 3,
            puncture = true;
        
        for (i = 0; i < ct.length; i += 2) {
            w = stego.indexWord((ct[i] << 8) | ct[i + 1]).toLowerCase();
            if (puncture && (sl == 0)) {
                w = w.substr(0, 1).toUpperCase() + w.substr(1, w.length);
            }
            
            //	If this is the last word, put a period after it unless we added a padding byte, in which case we end with a bang to so indicate.
            if (i == (ct.length - 2)) {
                w += padded ? "!" : ".";
            } else {	    
                if (puncture) {
                
                    //  Regular word.  Generate random but plausible punctuation
                    sl++;
                    if (sl >= (purng.nextInt(9) + 3)) {
                        var p = purng.nextInt(15), pu;
                        pu = (p <= 13) ? "." : ((p == 14) ? "?" : "!");
                        w += pu + " ";
                        sl = 0;
                        sc++;
                        if (sc >= parl) {
                            fpar = true;
                            sc = 0;
                        }
                    } else {
                        if (purng.nextInt(6) == 6) {
                            w += ",";
                        }
                    }
                }
            }
            if ((l.length + w.length + 1) > maxLine) {
                l = l.replace(/\s+$/, "");
                t += l + "\n";
                l = "";
            }
            if (l.length > 0) {
                l += " ";
            }
            l += w;
            if (fpar) {
                l = l.replace(/\s+$/, "");
                t += l + "\n\n";
                l = "";
                fpar = false;
                parl = purng.nextInt(8) + 2;
            }
        }
        t += l;
        delete purng;
        return t;
    },
    
    //! Decode text from words
    seekText : function(tag, b64) {
        
        var ct = new Array(), padded = false;
        
        // Precompute table of cumulative words before those of a given length.
        var awords = new Array(), i, j;
        j = 0;
        for (i = minw; i <= maxw; i++) {
            awords[i] = j;
            j += nwords[i];
        }
        
        var t = tag, n = 0, v;
        var wpat = /\W*(\w+)\w*/i;
        while (wpat.test(t)) {
            //	Extract next word from text and determine its length
            t = t.replace(wpat, "");
            var w = RegExp.$1;
            var l = w.length;
            
            //	Look it up in the list of words of this length
            w = w.substring(0, 1).toUpperCase() +
                   w.substring(1, w.length);
            v = cwords[l].indexOf(w);
            if (v >= 0) {
                v = (v / l) + awords[l];
            }
            if (v == -1) {
                stego.dump("Bogus word", w);
            } else {
                ct[n++] = (v >> 8) & 0xFF;
                ct[n++] = v & 0xFF;
            }
        }
        
        if (t.indexOf("!") != -1) {
            padded = true;
            ct.pop();
            n -= 1;
        }
        
        if (b64)    return stego.byteArrayToBase64( ct );
        else        return stego.byteArrayToHex( ct );
    },
    
    
    /**
        Dump one or more (variable_name, value) pairs given as arguments
        to the function.
    */
    dump : function()
    {
        var t = "", i;
        
        for (i = 0; i < arguments.length; i += 2) {
            if (t.length > 0) {
                t += ", ";
            }
            t += arguments[i] + " = " + arguments[i + 1];
        }
        document.debug.log.value += t + "\n";
    },
    
    /**
        L'Ecuyer's two-sequence generator with a Bays-Durham shuffleon the back-end.  Schrage's algorithm is used to perform 64-bit modular arithmetic within the 32-bit constraints of JavaScript.
        
        Bays, C. and S. D. Durham.  ACM Trans. Math. Software: 2 (1976) 59-64.
        
        L'Ecuyer, P.  Communications of the ACM: 31 (1968) 742-774.
        
        Schrage, L.  ACM Trans. Math. Software: 5 (1979) 132-138.
    */
    //! Schrage's modular multiplication algorithm
    uGen : function(old, a, q, r, m) {      
        var t;
    
        t = Math.floor(old / q);
        t = a * (old - (t * q)) - (t * r);
        return Math.round((t < 0) ? (t + m) : t);
    },
    //! Return next raw value
    LEnext : function() {                   
        var i;
        this.gen1 = stego.uGen(this.gen1, 40014, 53668, 12211, 2147483563);
        this.gen2 = stego.uGen(this.gen2, 40692, 52774, 3791, 2147483399);
    
        /* Extract shuffle table index from most significant part
           of the previous result. */
    
        i = Math.floor(this.state / 67108862);
    
        // New state is sum of generators modulo one of their moduli
    
        this.state = Math.round((this.shuffle[i] + this.gen2) % 2147483563);
    
        // Replace value in shuffle table with generator 1 result
    
        this.shuffle[i] = this.gen1;
    
        return this.state;
    },
    
    //! Return next random integer between 0 and n inclusive
    LEnint : function(n) {
        var p = 1;
    
        //  Determine smallest p,  2^p > n
    
        while (n >= p) {
            p <<= 1;
        }
        p--;
    
        /*  Generate values from 0 through n by first masking
            values v from 0 to (2^p)-1, then discarding any results v > n.
            For the rationale behind this (and why taking
            values mod (n + 1) is biased toward smaller values, see
            Ferguson and Schneier, "Practical Cryptography",
            ISBN 0-471-22357-3, section 10.8).  */
    
        while (true) {
            var v = this.next() & p;
    
            if (v <= n) {
                return v;
            }
        }
    },
    
    //! Constructor.  Called with seed value
    LEcuyer : function(s) {
    var i;

    this.shuffle = new Array(32);
    this.gen1 = this.gen2 = (s & 0x7FFFFFFF);
    for (i = 0; i < 19; i++) {
        this.gen1 = stego.uGen(this.gen1, 40014, 53668, 12211, 2147483563);
    }

    // Fill the shuffle table with values

    for (i = 0; i < 32; i++) {
        this.gen1 = stego.uGen(this.gen1, 40014, 53668, 12211, 2147483563);
        this.shuffle[31 - i] = this.gen1;
    }
    this.state = this.shuffle[0];
    this.next = stego.LEnext;
    this.nextInt = stego.LEnint;
}

};