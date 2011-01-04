/**
 * Encrypted Facebook authentication and submission methods
 */

eFB = {
    /**
    * Load the preferences XPCOM module, used to share data (i.e. the access token) across
    * windows.
    */
    prefs : Components.classes["@mozilla.org/preferences-service;1"]
                    .getService(Components.interfaces.nsIPrefService).getBranch("extensions.efb."),
                    

    /**
    * Initialises the login process, i.e. opens the login prompt.
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
                            "scope=publish_stream,read_stream,offline_access,user_notes,user_photos,friends_photos,user_photo_video_tags,friends_photo_video_tags&" +
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
            var msg = eFB.encryptString(plaintext).readString(); // No encrytion yet

            var subject_tag = encodeURIComponent( "ˠ" );
            var params =    "access_token=" + eFB.prefs.getCharPref("token") +
                            "&message=" + msg +
                            "&subject=" + subject_tag;

            // Create a new note
            var http = new XMLHttpRequest();
            var url = "https://graph.facebook.com/me/notes";
            http.open("POST", url, true);
            // Send the proper header information along with the request
            http.setRequestHeader("Content-type", "text/html; charset=utf-8");
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
            
            eFB.initialise= lib.declare("initialise",
                                     ctypes.default_abi,
                                     ctypes.uint32_t  // return type
            );
            eFB.encryptString= lib.declare("c_encryptString",
                                     ctypes.default_abi,
                                     ctypes.char.ptr, // return type
                                     ctypes.char.ptr // parameter 1
            );
            eFB.decryptString= lib.declare("c_decryptString",
                                     ctypes.default_abi,
                                     ctypes.char.ptr, // return type
                                     ctypes.char.ptr // parameter 1
            );
            eFB.encryptFileInImage= lib.declare("c_encryptFileInImage",
                                     ctypes.default_abi,
                                     ctypes.uint32_t, // return type
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.uint32_t, // parameter 2
                                     ctypes.char.ptr, // parameter 3
                                     ctypes.char.ptr // parameter 4
            );
            eFB.decryptFileFromImage= lib.declare("c_decryptFileFromImage",
                                     ctypes.default_abi,
                                     ctypes.uint32_t, // return type
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.char.ptr // parameter 2
            );
            eFB.calculateBitErrorRate= lib.declare("c_calculateBitErrorRate",
                                     ctypes.default_abi,
                                     ctypes.uint32_t, // return type
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.char.ptr // parameter 2
            );
            eFB.close = lib.declare("close",
                                     ctypes.default_abi,
                                     ctypes.int32_t // return type
            );
            
            // Find the path for the extension and pass it to the library
            eFB.cache_dir   = addon.getResourceURI( "" ).path + "cache/";
            eFB.temp_dir    = addon.getResourceURI( "" ).path + "temp/";
            callback( eFB.cache_dir, eFB.temp_dir );
            
        } );
    },
    
    initialise : function() {},
    encryptString: function() {},
    decryptString: function() {},
    encryptFileInImage: function() {},
    decryptFileFromImage: function() {},
    calculateBitErrorRate: function() {},
    close: function() {},
    
    
    generateEncryptedPhoto : function(s) {
        
        window.alert( eFB.encryptFileInImage( "aaaaaaaa00000001000000010000000100000001000000010000000100000001000000010000000100000001000000010000000100000001000000010000000100000001",
                                            1,
                                            "/home/chris/Desktop/hidden.jpg",
                                            "/home/chris/Desktop/out.bmp"
                                           ));
        
        window.alert( eFB.decryptFileFromImage(
                                            "/home/chris/Desktop/out.jpg",
                                            "/home/chris/Desktop/hidden2.jpg"
                                   ));
        window.alert( eFB.calculateBitErrorRate( "/home/chris/Desktop/hidden.jpg",
                                            "/home/chris/Desktop/hidden2.jpg"
                                   ));

    },
    
    uploadPhoto : function(path, album_id, callback) {
        
        // Create a form
        var form = content.document.createElement("form");
        
        // Create a file input element
        var file_input = content.document.createElement("input");
        file_input.setAttribute("type", "file");
        file_input.setAttribute("name", "source");
        file_input.value = path;
        
        // Create an access token input element
        var token_input = content.document.createElement("input");
        token_input.setAttribute("type", "text");
        token_input.setAttribute("name", "access_token");
        token_input.value = eFB.prefs.getCharPref("token");
        
        // Create a submit element
        var submit_input = content.document.createElement("input");
        submit_input.setAttribute("type", "submit");
        submit_input.setAttribute("name", "submit_button");
        submit_input.value = "Submit me";
        
        // Add the inputs to the form
        form.appendChild( file_input );
        form.appendChild( token_input );
        form.appendChild( submit_input );
        
        // Add action and method attributes
        form.setAttribute("id", "efbForm");
        form.action = "https://graph.facebook.com/" + album_id + "/photos";
        form.method = "POST";
        form.enctype = "multipart/form-data";
        
        // Add the form to the document body
        content.document.body.appendChild(form);
        
        // Send the form
        form.submit();

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
                        note = eFB.decryptString(note).readString();
                        
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
    cache : {},
    
    /**
     * And also a cache of (possible) image IDs and their status
     * 
    **/
    img_cache : {}

};

// Load the C/C++ binary library functions
// ensure that the callback function (i.e. the C/C++ library initilisation)
// is passed the extension path.
eFB.loadLibs( function( cache_dir,temp_dir ) {eFB.initialise();} );