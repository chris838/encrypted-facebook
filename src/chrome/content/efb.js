/**
    Encrypted Facebook authentication, identity management and message/image submission functions.
*/
eFB = {
    /**
        Load the preferences XPCOM module, used to share data (i.e. the access token) across windows.
    */
    prefs : Components.classes["@mozilla.org/preferences-service;1"]
                    .getService(Components.interfaces.nsIPrefService).getBranch("extensions.efb."),
    /**
        Define some constanst that will be used throughout.
    */
    // Subdirectories for storing files
    cache_dir   :"cache/",
    temp_dir    :"temp/",
    keys_dir    :"keys/",
    // User key filenames
    privkey_file : "user.key",
    pubkey_file : "user.pubkey",
    // Tag formats
    pubkey_start : "ᐊ✇",
    pubkey_end : "ᐅ",
    msg_start : "ᐊ✆",
    msg_end : "ᐅ",
    
    /**
        Initialise the login process and open the login prompt. Obtain a Facebook ID from the user and initialise the C++ libary with it. The process is completed when the login success url is detected, at which point we intercept the access token.
    */ 
    login : function(aEvent) {
    
        if ( !eFB.prefs.getBoolPref("loggedIn") ) {
            // If we aren't already logged in, open a login window
            eFB.api_id = window.prompt("Please enter your Facebook ID." , "146278748752732");
            // Load the C/C++ binary library functions.
            eFB.loadLibrary( function() {
                eFB.initialise( eFB.api_id, eFB.working_dir );
            } );
            // Create the login url
            var login_url = "https://graph.facebook.com/oauth/authorize?" +
                            "client_id=" + eFB.api_id + "&" +
                            "redirect_uri=http://www.facebook.com/connect/login_success.html&" +
                            "type=user_agent&" +
                            "scope=offline_access,user_notes,user_photos,friends_photos,user_photo_video_tags,friends_photo_video_tags,user_about_me,friends_about_me&" +
                            "display=popup";
            window.open( login_url, "fb-login-window", "centerscreen,width=350,height=250,resizable=0");
        } else {
            // And if we are, nothing to do
            //window.alert( "You are already logged in." );
            // TODO - this just for debugging
            //return;
            // Load the C/C++ binary library functions.
            eFB.api_id = "146278748752732";
            eFB.loadLibrary( function() {
                eFB.initialise( eFB.api_id, eFB.working_dir );
            } );
        }
    },
    
    /**
        Log the application out of Facebook and close the library.
    */
    logout : function(aEvent) {
    
        if ( !eFB.prefs.getBoolPref("loggedIn") ) {
            // If we aren't already logged in, nothing to do
            window.alert("You are already logged out.");
            return;

        } else {
            // Otherwise simply wipe token and status variables
            eFB.prefs.setBoolPref("loggedIn", false);
            eFB.prefs.setCharPref("token", "NO_TOKEN");
            // And close the C++ libary
            eFB.close();
            window.alert("You are now logged out from Facebook");
            return;
        }
    },
    
    
    /**
        Open the C/C++ extension components.
    */
    loadLibrary : function( callback ) {
        var MY_ID = "efb@cl.cam.ac.uk";  
        Components.utils.import("resource://gre/modules/AddonManager.jsm");
        AddonManager.getAddonByID(MY_ID, function(addon) {
            Components.utils.import("resource://gre/modules/ctypes.jsm");
            var lib = ctypes.open( addon.getResourceURI( "components/libtest.so" ).path );
            
            eFB.initialise= lib.declare("initialise",
                                     ctypes.default_abi,
                                     ctypes.uint32_t, // return type
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.char.ptr // parameter 2
            );
            eFB.generateIdentity= lib.declare("c_generateIdentity",
                                     ctypes.default_abi,
                                     ctypes.uint32_t, // return type                                     
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.char.ptr, // parameter 2
                                     ctypes.char.ptr // parameter 3
            );
            eFB.loadIdentity= lib.declare("c_loadIdentity",
                                     ctypes.default_abi,
                                     ctypes.uint32_t, // return type                                     
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.char.ptr, // parameter 2
                                     ctypes.char.ptr // parameter 3
            );
            eFB.loadIdKeyPair= lib.declare("c_loadIdKeyPair",
                                     ctypes.default_abi,
                                     ctypes.uint32_t, // return type                                     
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.char.ptr // parameter 2
            );
            eFB.encryptString= lib.declare("c_encryptString",
                                     ctypes.default_abi,
                                     ctypes.char.ptr, // return type
                                     ctypes.char.ptr, // parameter 1
                                     ctypes.char.ptr // parameter 2
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
                                     ctypes.char.ptr, // parameter 2
                                     ctypes.char.ptr // parameter 3
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
            
            // Find the path for the extension working directory and pass it to the library
            eFB.extension_dir = addon.getResourceURI( "" ).path;
            eFB.working_dir = eFB.extension_dir + eFB.api_id + "/";
            callback();
        } );
    },
    
    // C++ library function placeholders.
    initialise : function() {},
    generateIdentity : function() {},
    loadIdentity : function() {},
    loadIdKeyPair : function() {},
    encryptString: function() {},
    decryptString: function() {},
    encryptFileInImage: function() {},
    decryptFileFromImage: function() {},
    calculateBitErrorRate: function() {},
    close: function() {},
    
    /**
        Generate a new private/public key pair and store locally. This will wipe any current keys - be warned as loss of your private key means you lose access to all data encrypted with the corresponding public key.
    */
    createId : function(aEvent) {
        // Check if we already have an identity stored
        var priv = eFB.getFileObject( eFB.working_dir + eFB.keys_dir + eFB.privkey_file );
        var pub = eFB.getFileObject( eFB.working_dir + eFB.keys_dir + eFB.pubkey_file );
        try {
            foo = priv.size;
            if (!window.confirm("A private key was found in your profile folder. This operation will overwrite you current key information. Without your private key you will be permanently unable to access prior encrypted material. We strongly recommend you backup your private key if you have not done so already. Are you sure you wish to continue?")) {
                window.alert( "Operation cancelled.");
                return;
            }
        } catch(e)  {if ( e.name != "NS_ERROR_FILE_TARGET_DOES_NOT_EXIST" ) throw e;}
        try {
            foo = pub.size;
            if (!window.confirm("A public key was found in your profile folder. This operation will overwrite you current key information. Are you sure you wish to continue?")) {
                window.alert( "Operation cancelled.");
                return;
            }
        } catch(e)  {if ( e.name != "NS_ERROR_FILE_TARGET_DOES_NOT_EXIST" ) throw e;}
        
        // We need a passphrase to lock the file
        pass = window.prompt("Please enter a password to keep your key information safe. Note that forgotten passwords cannot be recovered.");
        
        // Go ahead and create the new (local) identity
        if (eFB.generateIdentity( eFB.keys_dir+eFB.privkey_file, eFB.keys_dir+eFB.pubkey_file, pass) == 0)
        window.alert("Generation successful");
        else window.alert("Error: key generation failed.");
    },
    
    /**
        Upload local key information to Facebook. This will enable others to include you as a recipient in encrypted communications.
    */
    uploadId : function(aEvent) {
        
        // Check if the key is present on disk and load in to a string. Then find and delete existing keys, and finally append the new key.
        var priv = eFB.getFileObject( eFB.working_dir + eFB.keys_dir + eFB.privkey_file );
        var pub = eFB.getFileObject( eFB.working_dir + eFB.keys_dir + eFB.pubkey_file );
        try {foo = priv.size;}
        catch(e)  {
            if ( e.name != "NS_ERROR_FILE_TARGET_DOES_NOT_EXIST" ) throw e;
            window.alert("A private key file was not found on you local system. Operation will abort.");
            return;
        }
        try {
            foo = pub.size;
            reader = new FileReader();
            reader.addEventListener("loadend", function(e) {
                // Key was found on disk and loaded succesfully
                // Download "bio" and search for existing keys
                eFB.abort = false;
                eFB.downloadProfileAttribute("bio", findExistingKeys);
        }, false);
            reader.addEventListener("error", function(e) {window.alert("Error occured reading public key file");}, false);
            reader.readAsText(pub);
        } catch(e)  {
            if ( e.name != "NS_ERROR_FILE_TARGET_DOES_NOT_EXIST" ) throw e;
            window.alert("A public key file was not found on you local system. Operation will abort.");
            return;
        }
        
        // (Function to) check if one or more public keys are already present (and try to delete them).
        function findExistingKeys(biostring) {
            var rx = new RegExp( eFB.pubkey_start + "[0-9a-z\\+/\\-\\s\\n]*" + eFB.pubkey_end, "gim");
            biostring = biostring.replace( rx , function(pubkey) {
                    if (window.confirm("An existing public key was found on your Facebook profile. Do you wish to delete this key before continuing?")) {
                        return "";
                    } else {eFB.abort=true; return pubkey;}
                }
            )
            if (eFB.abort) {
                window.alert("One or more existing public keys were found on your profile and not removed. Check that you are signed in to the correct account, otherwise please remove this key and restart operation.");
            } else {
                // We can continue.
                appendNewKey(biostring);
            }
        }
        
        // (Function to) append the key on to the bio.
        function appendNewKey(biostring) {
            var new_biostring = biostring + "\n\n" + eFB.pubkey_start + reader.result + eFB.pubkey_end;
            //eFB.uploadProfileAttribute("bio", new_biostring, function() {window.alert("Public key uploaded successfuly.");} );
            // BEGIN NASTY HACK --------------------------------------------------------------->
            // Facebook API doesn't let us make updates to the users profile (including the biography) so we create an iFrame and do it manually. What a mess.
            window.alert("This will upload the public key through your browser. Please do not interrupt the process by using your browser. If asked to 'leave the page' then please click 'yes'. You will receive confirmation when the process is complete. If you do not receive confirmation within 10 seconds, please try again. Click OK to begin.");
            ifrm = content.document.createElement("iframe");
            ifrm.setAttribute('style', 'display:none;');
            ifrm.style.width = 10+"px"; 
            ifrm.style.height = 10+"px";
            ifrm.setAttribute("id","unique_iframe_010101");
            content.document.body.appendChild(ifrm);
            ifrm.onload = function() {
                doc = content.document.getElementById('unique_iframe_010101').contentWindow.document;
                if (doc.getElementsByName("about_me")[0]==undefined) return;
                doc.getElementsByName("about_me")[0].innerHTML = new_biostring;
                doc.getElementById("ep_form").submit();
                window.alert("Public key has been uploaded to profile.");
                setTimeout( function() {content.document.body.removeChild(ifrm);}, 5000);
            };
            ifrm.src = "http://www.facebook.com/editprofile.php";
            // END NASTY HACK ----------------------------------------------------------------->
        }
    },
    
    /**
        Download a specified attribute from the user's Facebook profile and pass it as a variable to the callback function provided.
    */
    downloadProfileAttribute : function(attribute, callback) {
        var params = "access_token=" + eFB.prefs.getCharPref("token");
        var url = "https://graph.facebook.com/me?" + params;
        var xhr= new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.onreadystatechange = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Extract the "about me" body from the response
                    callback( eval( '(' + xhr.responseText + ')' )[attribute] );
                } else {
                    window.alert("Error sending request: " + xhr.responseText);
                }
            }
        }
        xhr.send();
    },
    
    /**
        Upload a given string and write it to the specified attribute on the users profile. On completion call the callback function.
    */
    uploadProfileAttribute : function(attribute, value, callback) {
        var params =    "access_token=" + eFB.prefs.getCharPref("token") + "&" +
                        attribute + "=" + "sadfj asldjf asldk jf";    
        var url = "https://graph.facebook.com/me";
        var xhr= new XMLHttpRequest();
        xhr.open("POST", url, true);
        xhr.setRequestHeader("Content-type", "text/html; charset=utf-8");
        xhr.setRequestHeader("Content-length", params.length );
        xhr.setRequestHeader("Connection", "close");
        xhr.onreadystatechange = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Attribute should have been updated, finished.
                    callback();
                } else {
                    window.alert("Error sending request: " + xhr.responseText);
                }
            }
        }
        xhr.send(params);
    },
    
    /**
        Get a local file object by creating an invisible form element....no, I don't know why either.
    */
    getFileObject : function(path) {
        // Create an invisible form
        var form = content.document.createElement("form");
        form.setAttribute('style', 'display:none;');
        
        // Create a file input element
        var file_input = content.document.createElement("input");
        file_input.setAttribute("type", "file");
        file_input.setAttribute("id", "fileElem");
        file_input.value = path;
        
        // Add input to the form
        form.appendChild( file_input );
        
        // Add the form to the document body
        content.document.body.appendChild(form);
        
        // Grab the file handle
        var file = content.document.getElementById('fileElem').files[0];
        
        // Now delete the form object
        content.document.body.removeChild(form);
        
        return file;
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
    
    generateEncryptedPhoto : function(s) {
    
        window.alert( eFB.generateIdentity( "keys/user.key", "keys/user.pubkey", "password" ) );
        
        window.alert( eFB.loadIdentity( "keys/user.key", "keys/user.pubkey", "password" ) );
        
        window.alert( eFB.loadIdKeyPair( "11111111", "keys/11111111.pubkey" ) );
        window.alert( eFB.loadIdKeyPair( "123456788888888", "keys/123456788888888.pubkey" ) );
        window.alert( eFB.loadIdKeyPair( "1234567887654321", "keys/1234567887654321.pubkey" ) );
        
        msg = "Here is my message....";
        window.alert(msg);
        msg = eFB.encryptString( "1234567887654321;11111111;", msg ).readString();
        window.alert(msg);
        msg = eFB.decryptString( msg ).readString();
        window.alert(msg);
    
    /*
        window.alert( eFB.encryptFileInImage( "1234567887654321;11111111;",
                                            "/home/chris/Desktop/hidden.jpg",
                                            "/home/chris/Desktop/out.bmp"
                                           ));
        
        window.alert( eFB.decryptFileFromImage(
                                            "/home/chris/Desktop/out.bmp",
                                            "/home/chris/Desktop/hidden2.jpg"
                                   ));
        
        window.alert( eFB.calculateBitErrorRate(
                                            "/home/chris/Desktop/hidden.jpg",
                                            "/home/chris/Desktop/hidden2.jpg"
                                   ));
    */
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