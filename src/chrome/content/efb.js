/**
 * Encrypted Facebook authentication and submission methods
 */

eFB = {
    /**
    * Load the preferences XPCOM module, used to share data (i,e, the access token) across
    * windows.
    */
    prefs : Components.classes["@mozilla.org/preferences-service;1"]
                    .getService(Components.interfaces.nsIPrefService).getBranch("extensions.efb."),
                    

    /**
    * Initialises the log in process, i.e. opens the login prompt.
    * The process is completed when the login success url is detected,
    * at which point we intercept the access token.
    */ 
    login : function(aEvent) {
    
        if ( !eFB.prefs.getBoolPref("loggedIn") ) {
            // If we aren't already logged in, open a login window
            var api_id =    "146278748752732";
            var login_url = "https://graph.facebook.com/oauth/authorize?" +
                            "client_id=" + api_id + "&" +
                            "redirect_uri=http://www.facebook.com/connect/login_success.html&" +
                            "type=user_agent&" +
                            "scope=publish_stream,offline_access,user_about_me,friends_about_me&" +
                            "display=popup";
            window.open( login_url, "fb-login-window", "centerscreen,width=350,height=250,resizable=0");
        } else {
            // And if we are, nothing to do
             window.alert( eFB.prefs.getCharPref("token") );
            return;
        }
    },
    
    /**
    * Log out from Facebook
    */
    logout : function(aEvent) {
    
        if ( !eFB.prefs.getBoolPref("loggedIn") ) {
            // If we aren't already logged in, nothing to do
            return;

        } else {
            // Otherwise simply wipe token and status variables
            eFB.prefs.setBoolPref("loggedIn", false);
            eFB.prefs.setCharPref("token", "NO_TOKEN");
            
            window.alert("You are now logged out from Facebook");
            return;
        }
    },
    
    /**
    * Submits a note, returning a tag which links to the notes contents 
    * 
    **/
    submit : function(aEvent) {
        if ( eFB.prefs.getBoolPref("loggedIn") ) {
            
            // The variables we will submit
            var plaintext = document.getElementById("efb-text").value;
            //
            var msg = "Sample post thing";//encodeURIComponent( plaintext ); // No encrytion yet
            var subject_tag = encodeURIComponent( "<ˠ>" );
            var params =    "access_token=" + eFB.prefs.getCharPref("token") +
                            "&message=" + msg +
                            "&subject=" + subject_tag;
            
            // Create a new note
            var http = new XMLHttpRequest();
            var url = "https://graph.facebook.com/me/notes";
            http.open("POST", url, true);
            // Send the proper header information along with the request
            http.setRequestHeader("Content-type", "application/x-www-form-urlencoded");
            http.setRequestHeader("Content-length", params.length);
            http.setRequestHeader("Connection", "close");
            //
            http.onreadystatechange = function() {//Call a function when the state changes.
                    if(http.readyState == 4) {
                        if (http.status == 200) {
                            //var id = parseInt( eval( '(' + http.responseText + ')' ).id );
                            var id = plaintext;
                            window.alert(id);
                            window.alert( eFB.hexToBase128(id) );
                            window.alert( eFB.base128ToHex( eFB.hexToBase128(id) ) );
                        } else {
                            window.alert("Error sending request, " + http.responseText);
                        }
                    }
            }
            http.send(params);
 
           
        } else {
            window.alert("You aren't logged in to Facebook");
            return;
        }
    },

    /**
    * Takes an integer x and returns a UTF-8 Base 128 representation.
    * This means that only 1 bit of overhead is required for every 7 bits.
    * 
    **/
    intToBase128 : function( x ) {
                
        var y = 0; var r = "";
        
        // Repeat until no more bits left to encode
        while (x > 0) {
        
            // We take 7 bits from the (least significant) end of the
            // integer.
            y = x & 127;
            
            // We can map y to a byte between 1000 000 and 1111 1111
            r = String.fromCharCode(y) + r;
            
            // Then we shift x seven bits to the right
            x = x >> 7;
        }
        
        return r;
        
    },

    /**
    * Takes a hexadecimal string and returns a UTF-8 Base 128 representation.
    * This means that only 1 bit of overhead is required for every 7 bits.
    * 
    **/
    hexToBase128 : function( x ) {
                
        var y = ""; var r = "";
        
        // Repeat until no more bits left to encode
        while (x.length > 0) {
        
            // Take the last hex 7 digits. If less than 7, pad with '0'
            while (x.length < 7) x = '0' + x;
            y = x.substr( x.length-7 );
            
            // Calculate the 4 UTF8 characters corresponding to the 7 hex digits of y
            window.alert("about to base: " + y);
            r = eFB.intToBase128( parseInt(y,16) ) + r;
            window.alert("we got: " + eFB.intToBase128( parseInt(y,16) ));
            
            // Remove the last 7 characters from x
            x = x.substring(0, x.length-7);
        }
        
        return r;
        
    },

    /**
    * Takes a Base 128 UTF-8 string and returns an integer
    * 
    **/
    base128ToInt : function( x ) {
        
        var y = ""; var r = 0;
        
        // Repeat until no more characters remain
        while (x.length > 0) {
            
            // Take the first character of x
            y = x.charAt( 0 );
            
            // Shift our current result 7 places to the left
            // and XOR in the last 7 bits of y
            r = r << 7;
            r = r ^ y.charCodeAt(0);
            
            // Remove the first character of x
            x = x.substr(1, x.length-1);
            
        }
        
        return r;
    },
    
    /**
    * Takes a Base 128 UTF-8 string and returns a hex string
    * 
    **/
    base128ToHex : function( x ) {
        
        var y = ""; var r = "";
        
        // Repeat until no more characters remain
        while (x.length > 0) {
            
            // Take the last 8 UTF8 characters. If less than 8,
            // pad with null characters (charCode = 0)
            while (x.length < 8) x = x + String.fromCharCode(0);
            y = x.substr( x.length-8 );
            
            // Calculate the 7 hex digits corresponding to
            // the 8 UTF8 characters
            r = eFB.base128ToInt( y ).toString(16) + r;
            
            // Remove the last 8 characters
            x = x.substr(0, x.length-8);
            
        }
        
        return r;
    }

};

