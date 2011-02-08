/**
 * Encrypted Facebook page interception
 */

pc = {

    /**
        HTML code for the friend selector popup
    */
    selector_className : 'generic_dialog pop_dialog full_bleed',
    selector_html : "<div class='generic_dialog pop_dialog full_bleed'><div style='top: 0px; width: 467px;' class='generic_dialog_popup'><div class='pop_container_advanced'><div class='pop_content' id='pop_content'><h2 class='dialog_title'><span>Select Recipients</span></h2><div class='dialog_content'><div class='dialog_body'><div class='ListEditor_Container'><form id='friend_suggester_popup_form' action='/'><div id='fb_multi_friend_selector' class='resetstyles num_cols_3'><div id='fb_multi_friend_selector_wrapper' class='clearfix' style><div id='fb_mfs_container'><div id='fb_mfs_body'><div id='fb_multi_friend_selector_notice' style='display:none;'></div><div id='fs_current_filter' style='display:none;' class='clearfix'></div><ul id='friends' style='height:280px;'><div id='all_friends'>" +
  
 "</div></ul></div></div></div></div></form></div></div><div class='dialog_buttons clearfix'><label class='uiButton uiButtonLarge uiButtonConfirm'><input type='button' value='Send Message' name='save'></label><label class='uiButton uiButtonLarge '><input type='button' value='Cancel' name='cancel'></label></div></div></div></div></div></div>",

    /**
        First handler looks for a login.
    */
    onLoadLoginHandler : function(event) {
        // If we *are* logged in, nothing to do
        if (eFB.prefs.getBoolPref("loggedIn")) return;
        
        // this is the content document of the loaded page.  
        var doc = event.originalTarget;  
        
        if (doc instanceof HTMLDocument) {  
           // is this an inner frame?  
           if (doc.defaultView.frameElement) {  
                // Frame within a tab was loaded.  
                // Find the root document:  
                while (doc.defaultView.frameElement) {  
                  doc = doc.defaultView.frameElement.ownerDocument;  
                }
            }        
            
            // Check the URL for the dummy Facebook succesful login page
            var url = doc.defaultView.location.href;
            var rx=/facebook\.com\/connect\/login_success\.html#access_token/;
            if (rx.test(url)) {
                
                // First check we didn't get an error
                if ( url.indexOf("error_reason")  != -1 ) {
                    var e = url.slice( url.indexOf("error_reason") ).split('&')[0];
                    window.alert("Error logging in : " + e);
                    return;
                }
                
                // Save the access token
                eFB.prefs.setCharPref("token",
                                   url.slice( url.indexOf("access_token") ).split('&')[0].split('=')[1]
                                 );
                
                // Change status to logged in
                eFB.prefs.setBoolPref("loggedIn",true)
                
                // Close the window
                doc.defaultView.close();                
            }  
        }  
    },
    
    /**
        Second handler looks for Facebook pages that need intercepting
    */
    onLoadFacebookHandler : function(event) {
        // If we aren't logged in, nothing to do
        if (!eFB.prefs.getBoolPref("loggedIn")) return;
        
        // this is the content document of the loaded page.  
        var doc = event.originalTarget;  
        
        if (doc instanceof HTMLDocument) {  
            // is this an inner frame?  
            if (doc.defaultView.frameElement) {  
            // Frame within a tab was loaded.  
            // Find the root document:  
            while (doc.defaultView.frameElement) {  
                doc = doc.defaultView.frameElement.ownerDocument;  
            }  
                   
                // Check the URL for a Facebook page
                var url = doc.defaultView.location.href;
                var rx=/facebook\.com\//; // TODO - watch out for malicious code injections
                if (rx.test(url)) {
                    
                    // Try and replace any text
                    var page = doc.getElementById('content');
                    text = page.innerHTML;
                    // Find any tags, and call retrieve function on them
                    var rx2 = new RegExp( eFB.msg_start + "[0-9,a-f]*" + eFB.msg_end );
                    text = text.replace(
                        rx2, function(x) { return eFB.retrieveFromTag(doc,x); }
                    );
                    // Write back changes, but ONLY if there are any
                    // (since a rewrite will trigger another page load, and thus another parseHTML)
                    if (page.innerHTML != text) page.innerHTML = text;
                    // Add a few formatting changes to demark decrypted items
                    pc.applyFormatting(doc);
                    
                    // Now try replace any images with their plaintext
                    //pc.replaceImages(doc);
                    
                    // Insert user interface overlay
                    pc.iFaceOverlay(doc,url);
                }
            }
        }  
    },
    
    /**
        Format the messages which have been decrypted.
    */
    applyFormatting : function(doc) {
        // Get all the decrypted message list items and format them
        var list = doc.getElementsByClassName('note_complete');
        for (var i=0; i<list.length; i++) {
            var li = list[i];
            // Get the list item element within which the text is featured
            while ( li.tagName != "LI" && li.tagName != "li") li = li.parentNode;
            li.setAttribute("style",
                "border-top:1px dotted #c00; border-bottom:1px dotted #c00; background-color: #ffeeee;");
        }
    },
    
    /**
        Parse the Facebook page and insert Encrypted Facebook UI elements
    */
    iFaceOverlay : function(doc,url) {
        // Profile page might need public key add/remove controls
        if ( /profile\.php/.test(url) ) {
            // We are on the profile page and must try to insert public key grabber
            var idstr = url.match( /id=[0-9]*/i );
            if (idstr != null ) {
                var id = idstr[0].split("=")[1];
                // Check it isn't our ID
                if (id != eFB.id) pc.parseProfile(doc,id);
            }
        }
    
        // Message submission might need encrypted submission controls
        var mboxs = doc.getElementsByClassName('uiComposerMessageBox');
        for (var i = 0; i < mboxs.length; i++) {
            mbox = mboxs[i];
            // Get the submit button and button list.
            btlist = mbox.getElementsByTagName('ul').item(0);
            // Check we haven't added already
            if (btlist.lastChild.id != "efb_submit") {
                // Add in the new controls
                pc.addMessageControls(doc,url,btlist);
            }
        }
        
        // Comment submission might need encrypted submission controls
        var cboxs = doc.getElementsByClassName('uiUfiAddComment');
        for (var i = 0; i < cboxs.length; i++) {
            cbox = cboxs[i];
            // Get the submit button and button list.
            btlist = cbox.getElementsByTagName('label');
            // Check we haven't added already
            if ( btlist.length == 1 ) {
                // Add in the new controls
                pc.addCommentControls(doc,url,btlist[0].parentNode);
            }
        }
        
        // Picture upload might need encrypted picture upload conrtols
        // TODO
    },
        
    /**
        Parse a profile and decide if it needs a controls inserting.
    */  
    parseProfile : function(doc,id) {
        // Download the "bio" string
        eFB.downloadProfileAttribute( "bio", id, function(biostring) {
                if (biostring==null) return;
                // extract the first found public key
                var rx = new RegExp( eFB.pubkey_start + "[0-9a-z\\+/\\-\\s\\n]*" + eFB.pubkey_end, "im");
                var match = biostring.match(rx);
                if (match==null) return;
                else pc.addGrabKeyButton(doc,id,match[0]);
            }
        );
    },
    
    /**
        Add button to profile page.
    */
    addGrabKeyButton : function(doc,id,key) {
        // If we haven't already, add a grab key button
        if (doc.getElementById("profile_action_add_pubkey")==undefined && doc.getElementById("pagelet_header")!=undefined) { 
            box = doc.getElementById("pagelet_header").getElementsByClassName("rfloat")[0];
            box.innerHTML = '<a id="profile_action_add_pubkey" class="uiButton" rel="dialog" href="#"><i class="mrs img sp_6isv8o sx_79a2c2"></i><span class="uiButtonText">Add Public Key</span></a>'
                            + box.innerHTML;
            // Set the custom icon
            file = eFB.getFileObject( eFB.extension_dir + "images/icons.png" );                
            link = content.window.URL.createObjectURL(file);
            box.firstChild.firstChild.style.backgroundImage = "url("+link+")";
            // Check if we already have a key
            var path = eFB.working_dir + eFB.keys_dir + id + ".pubkey";
            file = eFB.getFileObject( path );
            try {var foo = file.size;}
            catch (e)  {
                if (e.name != "NS_ERROR_FILE_TARGET_DOES_NOT_EXIST") throw e;
                // File not found so add listener
                box.firstChild.addEventListener("click",
                    function(aEvent) {
                        // Save public key to disk
                        eFB.writeToFile( key , path);
                        doc.location.reload(true);
                        window.alert("Public key added to keyring.");
                    }, false );
                return;
            }
            // If we get here then file already exists
            box.firstChild.lastChild.innerHTML = "Remove Public Key";
            box.firstChild.addEventListener("click",
                function(aEvent) {
                    // Create a file handle
                    var file = Components.classes["@mozilla.org/file/local;1"]  
                                .createInstance(Components.interfaces.nsILocalFile);  
                    file.initWithPath( path );
                    file.remove(false);
                    doc.location.reload(true);
                    window.alert("Key removed from keyring.")
                }, false);
            return;
        }
    },
    
    /**
        Add a drop down of recipients and a submit button for encrypted submission.
    */
    addMessageControls : function(doc,url,btlist) {
        // Create a new button based on the submit button
        var bt = btlist.lastChild;
        var li = doc.createElement('li');
        li.id = "efb_submit";
        li.className = bt.className;
        li.innerHTML = "<label for='u983479_9' class='submitBtn uiButton uiButtonConfirm uiButtonLarge'><input value='Encrypt & Share' style='width:120px;'></label>";
        // Set click handler
        li.firstChild.addEventListener("click", function() {
            // Get the message
            var inputs = this.parentNode.parentNode.parentNode.parentNode.getElementsByTagName('input');
            for (var i=0; i<inputs.length; i++) {
                if (inputs[i].getAttribute("name")=="xhpc_message") {
                    var input = inputs[i];
                    break;
                }
            }
            // Define save callback function for selector
            var save_callback = function(tag) {
                input.setAttribute("value", tag);
                // submit the form
                var form =input.parentNode.parentNode.parentNode.parentNode.parentNode;
                form.submit();
            };
            // Define handler for save button for selector
            var save = function(e) {
                // get a list of selected recipient ids
                var list = doc.getElementById("all_friends").getElementsByTagName("li");
                var ids = [];
                for (var i=0; i<list.length; i++) {
                    if (list[i].className=="selected") ids.push( list[i].getAttribute( "userid" ) );
                }
                // create a note, tag will be passed to save_callback
                eFB.submitNote(ids, input.getAttribute("value"), save_callback);
            };
            // Pass text area and doc to friend selector
            pc.createFriendSelector(doc, save);
        }, false );
        
        // Append new button to the button list
        btlist.insertBefore( li, btlist.lastChild.nextSibling );
    },
    
    
    /**
        Create a friend selector popup. 
        @param callback Function to call when user clicks submit. Must be of the form f(x,y) where x is the message string and y is an array of recipient IDs.
    */
    createFriendSelector : function(doc, save) {
        // Set up some additional CSS needed for the selector
        var head = doc.getElementsByTagName('head')[0]
        //;
        var link = doc.createElement('link');
        link.rel = 'stylesheet';
        link.type = 'text/css';
        link.media = "all";
        link.href = 'http://static.ak.fbcdn.net/rsrc.php/ym/r/tRN44tXnLYU.css';
        //
        var link2 = doc.createElement('link');
        link2.rel = 'stylesheet';
        link2.type = 'text/css';
        link2.media = "all";
        link2.href = 'http://static.ak.fbcdn.net/rsrc.php/yi/r/3v5MpCq5nHR.css';
        //
        head.appendChild(link);
        head.appendChild(link2);
        
        // Create the "friend selector" popup
        var popup = doc.createElement('div');
        popup.id = "friend_selector";
        popup.className = pc.selector_className;
        popup.innerHTML = pc.selector_html;
        doc.getElementById('content').appendChild(popup);
        // get handle to the friends list
        var ul = doc.getElementById('all_friends');
        
        // Populate the selector
        var ids = pc.readLocalFriends();
        for (var i=0; i<ids.length;i++) {
            pc.requestFriendLiInfo(ids[i], doc, ul);
        }
        
        // Define handler for cancel button
        var cancel = function(e) {
            var popup = this.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode.parentNode;
            popup.parentNode.removeChild( popup );
        };
        

        // Attach handlers
        buttons = popup.getElementsByTagName('input');
        for (var i=0; i<buttons.length; i++) {
            if (buttons[i].name == "cancel") buttons[i].addEventListener("click", cancel, false);
            else if (buttons[i].name == "save") buttons[i].addEventListener("click", save, false);
        }
    },
    
    /**
        Read the list of local friends (whose public keys we have saved) from disk
    */
    readLocalFriends : function() {
        // Load an array of user ids from their public keys stored on disk
        var entries = eFB.getNsiFileObject( eFB.working_dir + eFB.keys_dir ).directoryEntries;
        var ids = [];
        while( entries.hasMoreElements() ) {
            var entry = entries.getNext();
            entry.QueryInterface(Components.interfaces.nsIFile);
            // Discount the users key file
            if ( entry.leafName.indexOf('user') == -1 )
                ids.push( entry.leafName.split('.')[0] );
        }
        return ids;
    },
    
    /**
        Add a drop down of recipients and a submit button for encrypted comment submission.
    */
    addCommentControls : function(doc,url,div) {
        // Create a new button based on the submit button
        var bt = div.lastChild;
        var lb = doc.createElement('label');
        //
        lb.id = "efb_submit_comment";
        lb.className = bt.className;
        lb.innerHTML = '<input value="Encrypt & Comment" style="width:130px">';
        lb.setAttribute('style', "margin: 5px 0 0 5px;" );
        
        // Set click handler
        lb.addEventListener("click", function() {
            // Get the message
            var input = doc.getElementsByClassName('DOMControl_shadow')[0];
            var msg = input.firstChild.nodeValue;
            // Define save callback function for selector
            var save_callback = function(tag) {
                // Get the id of the post
                var inputs = lb.parentNode.parentNode.parentNode.parentNode.parentNode.getElementsByTagName('input');
                for (var i=0; i<inputs.length; i++) {
                    if (inputs[i].getAttribute("name")=="feedback_params") {
                        var vals = inputs[i].getAttribute("value").split(',');
                        for (var j=0;j<vals.length;j++) {
                            if (vals[j].split(':')[0] == '"target_profile_id"') var pid = vals[j].split(':')[1];
                            if (vals[j].split(':')[0] == '"target_fbid"') var fbid = vals[j].split(':')[1];
                        }
                        var post_id = pid.replace(/"/g,'') + "_" + fbid.replace(/"/g,'');
                        break;
                    }
                }
                // Use Graph API to comment
                eFB.submitComment(tag, post_id);
                // Close the selector window
                var popup = doc.getElementById("friend_selector");
                popup.parentNode.removeChild( popup );
            };
            // Define handler for save button for selector
            var save = function(e) {
                // get a list of selected recipient ids
                var list = doc.getElementById("all_friends").getElementsByTagName("li");
                var ids = [];
                for (var i=0; i<list.length; i++) {
                    if (list[i].className=="selected") ids.push( list[i].getAttribute( "userid" ) );
                }
                // create a note, tag will be passed to save_callback
                eFB.submitNote(ids, msg, save_callback);
            };
            // Pass text area and doc to friend selector
            pc.createFriendSelector(doc, save);
        }, false );
        
        // Append new button to the button list
        div.insertBefore( lb, div.lastChild );
    },
    
    /**
        Make a call to the Facebook API requesting attributes to create a selector list item.
    */
    requestFriendLiInfo : function(fbid, doc, ul) {
        // Define a function to handle the response
        handle = function(atts) {
            // bit more work to get the image
            img_url = "https://graph.facebook.com/"+fbid+"/picture?"+"access_token="+eFB.prefs.getCharPref("token");
           // now create/add the list item
           pc.appendFriendLi(doc,ul, atts.id, atts.name, img_url);
        }
        
        // Get the attributes. They will be passed as an array to the function provided.
        eFB.downloadProfileAttributes(fbid, handle);
    },
       
    /**
        Create a friend selector list item and append it to the list provided.
    */
    appendFriendLi : function(doc, ul, sel_id, sel_name, sel_img) {
        // Generate the list item
        var li = doc.createElement('li');
        li.setAttribute("userid", sel_id );
        li.innerHTML = "<a href='javascript:void(0);' title='"+sel_name+"'><span class='square' style='background-image: url("+sel_img+");'><span></span></span><strong>"+sel_name+"</strong></a>";
        // Add to the list
        ul.appendChild(li);
        // click handler
        li.setAttribute("onClick",
            "if (this.className=='selected') this.className='';" +
            "else this.className='selected';"
        ); 
    },   
    replaceImages : function(doc) {
        
        // Try and replace any images
        // First try to get all facebook user images on the page
        var list = doc.getElementById("content").getElementsByTagName('img'); 
                        
        // Try and get rid of non-facebook graph api image objects
        var ar = [];
        for (var i=0; i<list.length; i++) {
            if (list.item(i).src.indexOf('fbcdn.net') != -1) {
                if          (list.item(i).src.indexOf('photos') != -1) {
                    ar.push(  list.item(i) );
                }
            }
        }
        
        // Try and replace any <i> tags with background images
        list = doc.getElementById("content").getElementsByTagName('i'); 
                        
        // Try and get rid of non-facebook graph api image objects
        var ar2 = [];
        for (var i=0; i<list.length; i++) {
            if (list.item(i).style.backgroundImage.indexOf('fbcdn.net') != -1) {
                ar2.push(  list.item(i) );
            }
        }
        
        // For each image, get the id - it will be the second group of numbers of the filename
        ar.forEach( function(x) {
                var y = x.src.split('/');
                var id = y[ y.length -1 ].split('.')[0].split('_')[1];                
        
                // We maintain a list of IDs that have been processed. For each ID there are
                // three possible statuses.
                if ( eFB.img_cache[id] != undefined ) {
                    switch(eFB.img_cache[id].status)
                    {
                        // 0: ID is not a valid image object ID or was not parsed correctly.
                        // Do nothing.
                        case 0 :
                            break;
                        
                        // 1: Decryption tried but failed - object is not encrypted or we don't have the key.
                        // Do nothing.
                        case 1 :
                            break;
                        
                        // 2: Object is encrypted, plaintext exists in cache.
                        // Replace image on page with cached plaintext (if we haven't already).
                        // If replacement is made, exit loop since this will trigger another parseHTML()
                        case 2 :
                            var file = eFB.getFileObject( eFB.cache_dir + id + '_plain.jpg' );
                            x.src = content.window.URL.createObjectURL(file);
                            x.removeAttribute('height');
                            break;
                        
                        // 3: Object is being processed (may or may not be decryptable).
                        // Add doc to list of docs that need updating on completion (if we haven't already).
                        // Replace image on page with loading image (if we haven't already).
                        // If replacement is made, exit loop since this will trigger another parseHTML()
                        case 3 :
                            if ( eFB.img_cache[id].docs.indexOf(doc) == -1 ) {
                                eFB.img_cache[id].docs.push(doc);
                            }
                            break;
                    }
                } else {
                    // Object does not exist in cache.
                    // Create pending cache entry (include reference to this doc) and initiate request.
                    // Replace image on page with loading image.
                    // Exit loop since replacement this will trigger another parseHTML()
                    eFB.img_cache[id] = { status : 3, docs : [] };
                    eFB.img_cache[id].docs.push( doc ); 
                    pc.getImageSRC(id);
                }
            }
        );
    
        // For each <i> tag, get the id - it will be the second group of numbers of the filename
        ar2.forEach( function(x) {
                var z = x.style.backgroundImage;
                var y = z.split('/');
                var id = y[ y.length -1 ].split('.')[0].split('_')[1];                
        
                // We maintain a list of IDs that have been processed. For each ID there are
                // three possible statuses.
                if ( eFB.img_cache[id] != undefined ) {
                    switch(eFB.img_cache[id].status)
                    {
                        // 0: ID is not a valid image object ID or was not parsed correctly.
                        // Do nothing.
                        case 0 :
                            break;
                        
                        // 1: Decryption tried but failed - object is not encrypted or we don't have the key.
                        // Do nothing.
                        case 1 :
                            break;
                        
                        // 2: Object is encrypted, plaintext exists in cache.
                        // Replace image on page with cached plaintext (if we haven't already).
                        // If replacement is made, exit loop since this will trigger another parseHTML()
                        case 2 :
                            var file = eFB.getFileObject( eFB.cache_dir + id + '_plain.jpg' );
                            var img = content.document.createElement("img");
                            img.src = content.window.URL.createObjectURL(file);
                            img.style.position = "absolute";
                            img.style.height = "116px";
                            img.style.width = "149px";
                            x.style.overflow = "hidden";
                            x.appendChild(img);
                            break;
                        
                        // 3: Object is being processed (may or may not be decryptable).
                        // Add doc to list of docs that need updating on completion (if we haven't already).
                        // Replace image on page with loading image (if we haven't already).
                        // If replacement is made, exit loop since this will trigger another parseHTML()
                        case 3 :
                            if ( eFB.img_cache[id].docs.indexOf(doc) == -1 ) {
                                eFB.img_cache[id].docs.push(doc);
                            }
                            break;
                    }
                } else {
                    // Object does not exist in cache.
                    // Create pending cache entry (include reference to this doc) and initiate request.
                    // Replace image on page with loading image.
                    // Exit loop since replacement this will trigger another parseHTML()
                    eFB.img_cache[id] = { status : 3, docs : [] };
                    eFB.img_cache[id].docs.push( doc ); 
                    pc.getImageSRC(id);
                }
            }
        );
    },
    
    getImageSRC : function(id) {
        
        // Create a new XML HTTP request
        var token = eFB.prefs.getCharPref("token");  
        var xhr = new XMLHttpRequest();
        xhr.open("GET", "https://graph.facebook.com/" + id + "?access_token=" + token);
        
        // Define the callback function
        var callback = function() {
            if(xhr.readyState == 4) {
                if (xhr.status == 200) {
                    // Check if we have a valid image
                    var response = eval( '(' + xhr.responseText + ')' );
                    if ( response == false ) {
                        // Not a valid image, write status 0 in cache
                        eFB.img_cache[id].status = 0;
                    } else { 
                        // Get the file using the source URL
                        var path = eFB.cache_dir + id + '.jpg' ;
                        var path2 = eFB.cache_dir + id + '_plain.jpg' ;
                        
                        // Define a progress listener for the download
                        var prog_listener = {
                            onProgressChange: function (aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress) {
                                
                            },
                            onStateChange: function(aWebProgress, aRequest, aStateFlags, aStatus) {
                                // If the download is finished
                                if (aStateFlags & Components.interfaces.nsIWebProgressListener.STATE_STOP) {
                                    // Try to decrypt the image
                                    if (!eFB.decryptFileFromImage( path, path2 )) {
                                        // Decryption successful
                                        eFB.img_cache[id].status = 2;
                                        eFB.img_cache[id].docs.forEach( pc.replaceImages );
                                    } else {
                                        // Decryption failed
                                        eFB.img_cache[id].status = 1;
                                    }
                                }
                            }
                        };
                        pc.SaveImageFromURL( response.source, path, prog_listener );
                    }
                } else {
                    window.alert("Error sending request, " + xhr.responseText);
                }
            }  
        }
        
        // Attach the callback function when the request returns
        xhr.onreadystatechange = function() {
            callback();
        }
        
        // Send the request
        xhr.send();
        
    },
    
    SaveImageFromURL : function(url,path,prog_listener) {
        
        var file = Components.classes["@mozilla.org/file/local;1"]  
                    .createInstance(Components.interfaces.nsILocalFile);  
        file.initWithPath( path );  
        var wbp = Components.classes['@mozilla.org/embedding/browser/nsWebBrowserPersist;1']  
                  .createInstance(Components.interfaces.nsIWebBrowserPersist);  
        var ios = Components.classes['@mozilla.org/network/io-service;1']  
                  .getService(Components.interfaces.nsIIOService);  
        var uri = ios.newURI(url, null, null);  
        wbp.persistFlags &= ~Components.interfaces.nsIWebBrowserPersist.PERSIST_FLAGS_NO_CONVERSION; // don't save gzipped  
        
        // add a progress listener
        wbp.progressListener = prog_listener;
        
        wbp.saveURI(uri, null, null, null, null, file);
    
    },
    
    
}

// On initialisation, set state to logged out and watch out for login
eFB.prefs.setBoolPref("loggedIn", false);
eFB.prefs.setCharPref("token", "NO_TOKEN");
gBrowser.addEventListener("load", pc.onLoadLoginHandler, true);
gBrowser.addEventListener("load", pc.onLoadFacebookHandler, true);