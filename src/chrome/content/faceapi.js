/**
    Encrypted Facebook API interface module
*/
faceapi = {
    /**
        Delete all elements through a graph API connection.
    */
    cleanConnection : function( connection ) {
        var params =    "access_token=" + eFB.prefs.getCharPref("token");
        var url = "https://graph.facebook.com/" + eFB.id + "/"+connection+"?" + params;
        var xhr= new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.onreadystatechange = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Go though each result and delete
                    var r = eFB.secureEval( xhr ).data;
                    for (var i=0; i<r.length; i++) {
                        setTimeout( eFB.deleteGraphApiObject( r[i].id ) , 0);
                        setTimeout( eFB.deleteGraphApiObject( r[i].id ) , 1000);
                        setTimeout( eFB.deleteGraphApiObject( r[i].id ) , 5000);
                        setTimeout( eFB.deleteGraphApiObject( r[i].id ) , 10000);
                    }
                } else {
                    window.alert("Error deleting wall posts: " + xhr.responseText);
                }
            }
        }
        xhr.send();
    },

    /**
        Delete a single Facebook Graph API object given its id
    */
    deleteGraphApiObject : function(id) {
        var params =     "access_token=" + eFB.prefs.getCharPref("token")
                    +   "&method=delete";
        var url = "https://graph.facebook.com/" + id + "?" + params;
        var xhr= new XMLHttpRequest();
        xhr.open("POST", url, true);
        xhr.send();
    },

    /**
        Check if we already have key information on disk an thus if we wish to overwrite
    */
    checkKeyOverwrite : function() {
        var path = eFB.working_dir + eFB.keys_dir + eFB.privkey_file ;
        var msg = "A private key was found in your profile folder. This operation will overwrite you current key information. Without your private key you will be permanently unable to access prior encrypted material. We strongly recommend you backup your private key if you have not done so already. Are you sure you wish to continue?";
        if (eFB.checkFileOverwrite( path, msg )) return 1;
        //
        var path = eFB.working_dir + eFB.keys_dir + eFB.pubkey_file ;
        var msg = "A public key was found in your profile folder. This operation will overwrite you current key information. Are you sure you wish to continue?";
        if (eFB.checkFileOverwrite( path, msg )) return 1;
        // If we're still here then go ahead and overwrite (or doesn't exist)
        return 0;
    },

    /**
        Check if a single file exists and if we wish to overwrite.
    */
    checkFileOverwrite : function(path, msg) {
        var file = eFB.getFileObject(path);
        // Try to open file
        try {
            foo = file.size;
            if (!window.confirm(msg)) {
                // Don't overwrite
                return 1;
            }
            // Overwrite
            return 0;
        } catch(e)  {if ( e.name != "NS_ERROR_FILE_TARGET_DOES_NOT_EXIST" ) throw e;}
        // Doesn't exist
        return 0;
    },


    /**
        Generate a new private/public key pair and store locally. This will wipe any current keys - be warned as loss of your private key means you lose access to all data encrypted with the corresponding public key.
    */
    createId : function(aEvent) {
        // Check if we already have an identity stored
        if (eFB.checkKeyOverwrite()) {
            window.alert("Operation aborted.");
            return;
        }

        // We need a passphrase to lock the file
        pass = window.prompt("Please select a password to keep your key information safe. This password is not related to your Facebook account password. IMPORTANT: forgetting your password will result in irrecoverable loss of data.");

        // Go ahead and create the new (local) identity
        if (eFB.generateIdentity( eFB.keys_dir+eFB.privkey_file, eFB.keys_dir+eFB.pubkey_file, pass) == 0)
        window.alert("Generation successful");
        else window.alert("Error: key generation failed.");
    },

    /**
        Upload local public key information to Facebook. This will enable others to include you as a recipient in encrypted communications.
    */
    uploadPk : function(aEvent) {

        // Check if the key is present on disk and load in to a string. Then find and delete existing keys. Finally, append the new key.
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
                eFB.downloadProfileAttribute("bio", "me", findExistingKeys);
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
            if (biostring==undefined) biostring="";
            var rx = new RegExp( eFB.pubkey_start + "[a-zA-Z0-9\'\,\.\!\-\?\n\r úíüáñóé]+" + eFB.pubkey_end, "gim");
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
            var new_biostring = biostring + eFB.generatePubKeyMsg(reader.result);
            //eFB.uploadProfileAttribute("bio", new_biostring, function() {window.alert("Public key uploaded successfuly.");} );
            // BEGIN NASTY HACK --------------------------------------------------------------->
            // Facebook API doesn't let us make updates to the users profile (including the biography) so we create an iframe and do it manually. What a mess.
            window.alert("This will upload the public key through your browser. Please do not interrupt the process by using your browser. If asked to 'leave the page' then please click 'yes'. You will receive confirmation when the process is complete. If you do not receive confirmation within 10 seconds, please try again. Click OK to begin.");
            ifrm = content.document.createElement("iframe");
            ifrm.setAttribute('style', 'display:none;');
            ifrm.style.width = 500+"px";
            ifrm.style.height = 500+"px";
            ifrm.setAttribute("id","unique_iframe_010101");
            content.document.body.appendChild(ifrm);
            ifrm2 = content.document.getElementById('unique_iframe_010101');
            ifrm2.src = "http://www.facebook.com/editprofile.php";
            var foo = function() {
                doc = content.document.getElementById('unique_iframe_010101').contentWindow.document;
                if (doc.getElementsByName("about_me")[0]==undefined) return;
                doc.getElementsByName("about_me")[0].innerHTML = new_biostring;
                doc.getElementById("ep_form").submit();
                window.alert("Public key has been uploaded to profile.");
                setTimeout( function() {content.document.body.removeChild(ifrm);}, 5000);
            };
            var bar = function(i) {
                if (ifrm2.contentWindow.document.readyState=="complete") {
                    foo();
                } else if (i<100) {
                    setTimeout( function() {bar(i+1);}, 100);
                } else window.alert("Failed. Please try again.");
            }
            bar(0);
            // END NASTY HACK ----------------------------------------------------------------->
        }
    },
    
    /**
        Generate a public key message ready to upload.
     */
    generatePubKeyMsg : function(pk) {
        // length of header and footer
        var x = eFB.pubkey_head.length;
        var y = eFB.pubkey_tail.length;
        pk = pk.substr(x, pk.length-(x+y) ); // remove header and footer
        return "\n\n" + eFB.pubkey_start + "\n\n" + stego.hideText(pk,true) + "\n\n" + eFB.pubkey_end;
    },
    
    /**
        Download public key from Facebook profile.
    */
    downloadPk : function(aEvent) {
        // Check if we already have a public key stored
        var path = eFB.working_dir + eFB.keys_dir + eFB.pubkey_file ;
        var msg = "A public key was found in your profile folder. This operation will overwrite you current public key. Are you sure you wish to continue?";
        if (eFB.checkFileOverwrite( path, msg )) {
            window.alert("Operation aborted.");
            return;
        }
        
        // Import the public key from Facebook
        eFB.downloadProfileAttribute("bio", "me", function(biostring) {
            // Extract key from string
            var rx = new RegExp( eFB.pubkey_start + "[a-zA-Z0-9\'\,\.\!\-\?\n\r úíüáñóé]+" + eFB.pubkey_end, "im");
            biostring.replace(rx , function(pubkey) {
                // Trim tags of either end
                pubkey = parsePubKeyMsg( pubkey );
                // Write to a file
                var path = eFB.working_dir + eFB.keys_dir + eFB.pubkey_file;
                eFB.writeToFile( pubkey, path);
            });
        });
    },
    
    /**
        Extract a public key from an encoded message.
    */
    parsePubKeyMsg : function( pubkey ) {
        // Remove start and end tags
        var msg = pubkey.substr(eFB.pubkey_start.length, pubkey.length-(eFB.pubkey_end.length+eFB.pubkey_start.length));
        // Remove break tags
        msg.replace( /<br>/ , "");
        return eFB.pubkey_head + stego.seekText( msg , true) + eFB.pubkey_tail;
    },

    //! Write string out to a file
    writeToFile : function(string, path) {
        // Create a file handle
        var file = Components.classes["@mozilla.org/file/local;1"]
                    .createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath( path );
        // Open a fostream object
        var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"].
                       createInstance(Components.interfaces.nsIFileOutputStream);
        // 0x02 writing, 0x08 create new file, 0x20 start at begining of file
        foStream.init(file, 0x02 | 0x08 | 0x20 , 0666, 0);
        var converter = Components.classes["@mozilla.org/intl/converter-output-stream;1"].
                        createInstance(Components.interfaces.nsIConverterOutputStream);
        converter.init(foStream, "UTF-8", 0, 0);
        converter.writeString(string);
        converter.close();
    },

    /**
        Export private key to a file.
    */
    exportPk : function(aEvent) {
        // Create a file picker component
        const nsIFilePicker = Components.interfaces.nsIFilePicker;
        var fp = Components.classes["@mozilla.org/filepicker;1"]
                 .createInstance(nsIFilePicker);
        fp.init(window, "Export Private Key", nsIFilePicker.modeSave);
        var rv = fp.show();
        // When user selects a file
        if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
            // Open the current private key file
            var src_file = eFB.getFileObject(eFB.working_dir + eFB.keys_dir + eFB.privkey_file);
            var privkey = src_file.getAsText("UTF-8");
            // Open destination file
            var dst_file = fp.file;
            // Open a fostream object
            var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"].
                           createInstance(Components.interfaces.nsIFileOutputStream);
            // 0x02 writing, 0x08 create new file, 0x20 start at begining of file
            foStream.init(dst_file, 0x02 | 0x08 | 0x20 , 0666, 0);
            var converter = Components.classes["@mozilla.org/intl/converter-output-stream;1"].
                            createInstance(Components.interfaces.nsIConverterOutputStream);
            converter.init(foStream, "UTF-8", 0, 0);
            converter.writeString(privkey);
            converter.close();
            window.alert( "Private key exported to: "+dst_file.path+"\n\nPlease keep this information safe." );
        }
    },

    /**
        Import a private key from a file. This will overwrite any current private key.
    */
    importPk : function(aEvent) {
        // Create the file picker
        const nsIFilePicker = Components.interfaces.nsIFilePicker;
        var fp = Components.classes["@mozilla.org/filepicker;1"]
                 .createInstance(nsIFilePicker);
        fp.init(window, "Export Private Key", nsIFilePicker.modeOpen);
        var rv = fp.show();
        // When user selects a file
        if (rv == nsIFilePicker.returnOK || rv == nsIFilePicker.returnReplace) {
            // Open the current private key file
            var src_file = eFB.getFileObject( fp.file.path );
            var privkey = src_file.getAsText("UTF-8");
            // Open destination file
            var dst_file = Components.classes["@mozilla.org/file/local;1"]
                            .createInstance(Components.interfaces.nsILocalFile);
            dst_file.initWithPath(eFB.working_dir + eFB.keys_dir + eFB.privkey_file);
            // Open a fostream object
            var foStream = Components.classes["@mozilla.org/network/file-output-stream;1"].
                           createInstance(Components.interfaces.nsIFileOutputStream);
            // 0x02 writing, 0x08 create new file, 0x20 start at begining of file
            foStream.init(dst_file, 0x02 | 0x08 | 0x20 , 0666, 0);
            var converter = Components.classes["@mozilla.org/intl/converter-output-stream;1"].
                            createInstance(Components.interfaces.nsIConverterOutputStream);
            converter.init(foStream, "UTF-8", 0, 0);
            converter.writeString(privkey);
            converter.close();
            window.alert( "Private key imported from: "+fp.file.path );
        }
    },

    /**
        Download a specified attribute from the user's Facebook profile and pass it as a variable to the callback function provided.
    */
    downloadProfileAttribute : function(attribute, id, callback) {
        var params = "access_token=" + eFB.prefs.getCharPref("token");
        var url = "https://graph.facebook.com/" + id + "?" + params;
        var xhr= new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.onreadystatechange = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Extract the "about me" body from the response
                    callback( eFB.secureEval( xhr )[attribute] );
                } else {
                    window.alert("Error sending request: " + xhr.responseText);
                }
            }
        }
        xhr.send();
    },

    /**
        Download a list of attributes from the user's Facebook profile and pass it as a variable to the callback function provided.
    */
    downloadProfileAttributes : function(id, callback) {
        var params = "access_token=" + eFB.prefs.getCharPref("token");
        var url = "https://graph.facebook.com/" + id + "?" + params;
        var xhr= new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.onreadystatechange = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Extract the "about me" body from the response
                    callback( eFB.secureEval( xhr ) );
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
        Get a local NSI file object.
    */
    getNsiFileObject : function(path) {
        var file = Components.classes["@mozilla.org/file/local;1"].
                   createInstance(Components.interfaces.nsILocalFile);
        file.initWithPath(path);
        return file;
    },

    /**
        Submits a note, returning a tag which links to the note's contents.
    **/
    submitNote : function(recipients,plaintext,callback) {
        if ( eFB.prefs.getBoolPref("loggedIn") ) {
            // Refresh public keys and load them in to the library.
            var r_string = eFB.refreshPubKeys(recipients);

            // Encrypt the message content
            var msg = eFB.encryptString(r_string,plaintext).readString();
            //
            
            var subject_tag = encodeURIComponent( eFB.note_title);
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
            http.onreadystatechange = function() {
                // Call a function when the state changes.
                if(http.readyState == 4) {
                    if (http.status == 200) {
                        // Generate a tag to link to the note
                        var id = parseInt( eFB.secureEval( http ).id ).toString(16);
                        var tag = eFB.generateTag( id );
                        callback( tag );
                        // Delete the post on the users feed
                        setTimeout( eFB.deleteNotePosts, 0);
                        setTimeout( eFB.deleteNotePosts, 100);
                        setTimeout( eFB.deleteNotePosts, 3000);
                        setTimeout( eFB.deleteNotePosts, 10000);
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
        Cycles through the suplied list of public keys, updating them from Facebook if possible. Returns a string which can be used when encrypting;
    */
    refreshPubKeys : function(recipients) {
        // Load the required public keys in to the library
        var r_string = "";
        for (var i=0; i<recipients.length; i++) {
            /* Ignore this for now
            // Download the "bio" string
            eFB.downloadProfileAttribute( "bio", recipients[i], function(biostring) {
                    if (biostring!=null) {
                        // extract the first found public key
                        var rx = new RegExp( eFB.pubkey_start + "[a-zA-Z0-9\'\,\.\!\-\?\n\r úíüáñóé]+" + eFB.pubkey_end, "im");
                        var match = biostring.match(rx);
                        // If we find a key update it, otherwise just have to move on.
                        if (match!=null) {
                            var path = eFB.keys_dir + recipients[i] + ".pubkey";
                            eFB.writeToFile( eFB.parsePubKeyMsg( match[0] ) , path);
                        }
                    }
                    
                    // Load the refreshed key in to the application
                    eFB.loadIdKeyPair( recipients[i], eFB.keys_dir + recipients[i] + ".pubkey");
                }
            );
            */
            // Load the refreshed key in to the application
            eFB.loadIdKeyPair( recipients[i], eFB.keys_dir + recipients[i] + ".pubkey");
            // Build up the recipient string
            r_string += recipients[i] + ";";
        }
        
        // return string to use for encryption
        return r_string;
    },
    
    /**
        Unlock and load key information from local storage in to the application. 
    */
    loadCryptoState : function(password) {
            // Load the user's keys in to the library
            eFB.loadIdentity(
                eFB.keys_dir + "user.key",
                eFB.keys_dir + "user.pubkey", password);
    },
    
    /**
        Submit a comment on the supplied post ID.
    */
    submitComment : function(comment, post_id) {
        // Parameters to send
        var params =    "access_token=" + eFB.prefs.getCharPref("token") +
                "&message=" + comment;
        // Create the XML HTTP request object
        var xhr = new XMLHttpRequest();
        var url = "https://graph.facebook.com/" + post_id + "/comments";
        xhr.open("POST", url, true);
        // Send the proper header information along with the request
        xhr.setRequestHeader("Content-type", "text/html; charset=utf-8");
        xhr.setRequestHeader("Content-length", params.length);
        xhr.setRequestHeader("Connection", "close");
        xhr.send(params);
    },

    /**
        Delete any posts about new eFB notes on the users wall.
    */
    deleteNotePosts : function() {
    // Delete posts on the wall
        var params =    "access_token=" + eFB.prefs.getCharPref("token");
        var url = "https://graph.facebook.com/" + eFB.id + "/feed?" + params;
        var xhr= new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.onreadystatechange = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Go though each result and delete
                    var r = eFB.secureEval( xhr ).data;
                    for (var i=0; i<r.length; i++) {
                        if (r[i].name == eFB.note_title ) {
                            eFB.deleteGraphApiObject( r[i].id );
                        }
                    }

                } else {
                    window.alert("Error deleting note wall posts: " + xhr.responseText);
                }
            }
        }
        xhr.send();
    },

    
    /**
        Given a list of recipients, generate an encrypted version of an image. Return the path of the (temporary) encrypted image.
    */
    generateEncryptedPhoto : function(recipients,target_path) {
        if ( eFB.prefs.getBoolPref("loggedIn") ) {
            
            // Refresh public keys and load them in to the library.
            var r_string = eFB.refreshPubKeys(recipients);
            
            // Create a temp image filename
            var rand = Math.floor( Math.random()*100000001 );
            var output_path = eFB.working_dir+eFB.temp_dir + rand + ".bmp";
            
            // Encrypt the image
            eFB.encryptFileInImage(r_string, target_path, output_path);
            
            return output_path;
        }
        return "NOT/LOGGED/IN";
    },
    
    /**
        Upload a photo to Facebook via Graph API.
    */
    uploadPhoto : function(path, aid, redirect) {
        
        // Get the actual album ID from aid
        eFB.getAlbumId(aid, function(album_id) {
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
            
            // Redirect browser
            setTimeout( function() {content.document.defaultView.location = redirect;}, 15000 );  
        });
    },

    /**
        Work out the actual album ID given the aid
    */
    getAlbumId : function(aid,callback) {
        var params = "access_token=" + eFB.prefs.getCharPref("token");
        var url = "https://graph.facebook.com/" + eFB.id + "/albums?" + params;
        var xhr= new XMLHttpRequest();
        xhr.open("GET", url, true);
        xhr.onreadystatechange = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Find the correct album and get the album ID
                    var r = eFB.secureEval( xhr ).data;
                    for (var i=0; i<r.length; i++) {
                        if (r[i].link.indexOf( aid ) != -1 ) {
                            // Found the album
                            callback( r[i].id );
                            return;
                        }
                    }
                } else {
                    window.alert("Error retrieving album list: " + xhr.responseText);
                }
            }
        }
        xhr.send();
    },
    
    /**
        Generate a tag given an id linking to a note
    */
    generateTag : function(id) {
        return eFB.msg_start + stego.hideText(id) + eFB.msg_end;
    },
    
    /**
        Retrieve the contents of a note given the tag
    */
    retrieveFromTag : function(doc, tag) {

        // extract ID from tag
        var id = parseInt( eFB.parseTag( tag ), 16);

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
                        var obj = eFB.secureEval( http );
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

                        // Replace the tags
                        for (var i=0; i < doclist.length; i++) eFB.replaceTags( doclist[i], id );
                        

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
            return "<span style='color: #c00;' class='note_complete note_pending_"+ id + "'>Loading. please wait...<\/span>";

        } else if ( eFB.cache[ id ].Status == "PENDING" ) {

            // If pending request, add window to doclist list so that it have its HTML
            // updated when the the HTTP request returns.
            var doclist = eFB.cache[ id ].Docs;
            if (doclist.indexOf( doc ) != -1 ) doclist.push( doc );
            return "<span style='color: #c00;' class='note_complete note_pending_"+ id + "'>Loading. please wait...<\/span>";

        } else {
            
            return "<span style='color: #0;' class='note_complete'>"+ eFB.cache[ id ] +"<\/span>";
            
        }
    },

    /**
        Retrieve a note ID from a tag.
    */
    parseTag : function( tag ) {
        return stego.seekText(
            tag.substring(eFB.msg_start.length, tag.length-eFB.msg_end.length)
        );
    },
    
    /**
        Parse a document's HTML, finding occurences of pending tag request and process appropriately.
    **/
    replaceTags : function(doc, id) {

        var toReplace = doc.getElementsByClassName( "note_pending_" + id );
        for (var i=0; i < toReplace.length; i++) {
            toReplace[i].innerHTML = eFB.cache[ id ];
            toReplace[i].setAttribute( "style", "color: #0;");
            toReplace[i].setAttribute( "class", "note_complete");
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
