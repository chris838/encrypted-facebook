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
                            "scope=publish_stream,offline_access,user_about_me,friends_about_me,user_notes&" +
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
            //var plaintext = document.getElementById("efb-text").value;
            //
            //var msg = encodeURIComponent( eFB.lib_binToUTF8(plaintext,plaintext.length).readString() ); // No encrytion yet
   
            var msg = eFB.encodeForUpload("");

            var subject_tag = encodeURIComponent( "ˠ" );
            var params =    "access_token=" + eFB.prefs.getCharPref("token") +
                            "&message=" + msg +
                            "&subject=" + subject_tag;

            // Create a new note
            var http = new XMLHttpRequest();
            var url = "https://graph.facebook.com/me/notes";
            http.open("POST", url, true);
            // Send the proper header information along with the request
            http.setRequestHeader("Content-type", "application/x-www-form-urlencoded; charset=utf-8");
            http.setRequestHeader("Content-length", params.length);
            http.setRequestHeader("Connection", "close");
            //
            http.onreadystatechange = function() {//Call a function when the state changes.
                    if(http.readyState == 4) {
                        if (http.status == 200) {
                            // Generate a tag to link to the note
                            var id = parseInt( eval( '(' + http.responseText + ')' ).id ).toString(16);
                            var tag = eFB.generateTag( id );
                            window.alert( tag );
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
    
    // Open the C/C++ extension components
    loadLibs : function( callback ) {

        var MY_ID = "efb@cl.cam.ac.uk";  
        Components.utils.import("resource://gre/modules/AddonManager.jsm");
        AddonManager.getAddonByID(MY_ID, function(addon) {
            Components.utils.import("resource://gre/modules/ctypes.jsm");
            var lib = ctypes.open( addon.getResourceURI( "components/libtest.so" ).path );
            
            eFB.init_lib= lib.declare("init_lib",
                                     ctypes.default_abi,
                                     ctypes.int32_t // return type
            );
            eFB.lib_binToUTF8= lib.declare("lib_binToUTF8",
                                     ctypes.default_abi,
                                     ctypes.char.ptr, // return type
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.uint32_t  //parameter 2
            );
            eFB.lib_UTF8ToBin= lib.declare("lib_UTF8ToBin",
                                     ctypes.default_abi,
                                     ctypes.char.ptr, // return type
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.uint32_t  //parameter 2
            );
            eFB.close_lib = lib.declare("close_lib",
                                     ctypes.default_abi,
                                     ctypes.int32_t // return type
            );

            callback();
        } );
    },
    
    init_lib : function() {},
    lib_binToUTF8: function() {},
    lib_UTF8ToBin: function() {},
    close_lib: function() {},
    
    encodeForUpload : function(s) {
        
        s = "";
        for (var i = 0xb0; i < (65536 + 0xb0); i++) s+= String.fromCharCode(i);        
        return s;
        
    },
    
    decodeFromDownload : function(s) {
        
        
        
    },
    
    generateTag : function(id) {
        return "ᐊ" + id + "ᐅ";
    },
    
    retrieveFromTag : function(doc, tag) {

        // extract ID from tag
        var id = parseInt( tag.substring(1, tag.length-1), 16);
        
        if (eFB.cache[ id ] == undefined) {
        
            // Begin request for note
            var params = "access_token=" + eFB.prefs.getCharPref("token");
            var http = new XMLHttpRequest();
            var url = "https://graph.facebook.com/" + id.toString(10) + "?" + params;
            http.open("GET", url, true);
    
            http.onreadystatechange = function() {//Call a function when the state changes.
                if (http.readyState == 4) {
                    if (http.status == 200) {
                        
                        // Decode the actual text from the note body
                        var obj = eval( '(' + http.responseText + ')' );
                        var note = obj.message;
                        var id = parseInt( obj.id, 10 );
                        // decode note
                        note = eFB.lib_UTF8ToBin(note,note.length).readString();
                        
                        // Copy the list of docs (if any) we need to refresh
                        var doclist = [];
                        if ( eFB.cache[ id ] != undefined & eFB.cache[ id ].Status == "PENDING" ) {
                            doclist = [].concat( eFB.cache[ id ].Docs );
                        }
                        
                        // save to cache for later use
                        eFB.cache[ id ] = note;
                        
                        // Add in a random delay to mimic exagerated network latency
                        setTimeout( function() {
                            // cycle through documents that need updating and replace tags
                            for (var i=0; i < doclist.length; i++) eFB.replaceTags( doclist[i], id );
                        }
                        , Math.floor(Math.random()*4000));
                        
                    } else {
                        
                        // Need to reset cache since HTTP reqest was unsuccesful
                        // TODO - cycle through and wipe any pending statuses
                        window.alert("Error sending request, " + http.responseText);
                    }
                }
            }
            
            // Set the cache to pending, add window to list that needs to have HTML updated
            eFB.cache[ id ] = { Status : "PENDING", Docs : [ doc ] };
            
            // Now send the request
            http.send(null);
            
            // Return a temporary tag, for now
            return "<span style='color: #f00;' class='note_pending_"+ id + "'>Loading. please wait...<\/span>";
        
        } else if ( eFB.cache[ id ].Status == "PENDING" ) {
            
            // If pending request, add window to doclist list so that it have its HTML
            // updated when the the HTTP request returns.
            var doclist = eFB.cache[ id ].Docs;
            if (doclist.indexOf( doc ) != -1 ) doclist.push( doc );
            return "<span style='color: #f00;' class='note_pending_"+ id + "'>Loading. please wait...<\/span>";
        
        } else {
            return "<span style='color: #0f0;' class='note_"+ id + "'>"+ eFB.cache[ id ] +"<\/span>";
        }
    },

    /**
     * Parse a document's HTML, finding occurences of pending tag request
     * and process appropriately.
     * 
    **/
    replaceTags : function(doc, id) {
                
        var toReplace = doc.getElementsByClassName( "note_pending_" + id );
        for (var i=0; i < toReplace.length; i++) {
            toReplace[i].innerHTML = eFB.cache[ id ];
            toReplace[i].setAttribute( "style", "color: #0f0;");
            toReplace[i].setAttribute( "class", "note_" + id);
        }

    },
    
    /**
     * We maintain a cache of previously retrieved and decrypted notes
     * 
    **/
    cache : {}

};

// Load the C/C++ binary library functions
eFB.loadLibs( function() {eFB.init_lib();} );