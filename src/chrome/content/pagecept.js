/**
 * Encrypted Facebook page interception
 */

// First handler looks for a login
var onPageLoad = function(event) {
    
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
            
            // Now remove the listener
            gBrowser.removeEventListener("load", this._loadHandler, true);
            
            // Close the window
            doc.defaultView.close();
            
        }  
    }  
}

var onPageLoad2 = function(event) {
    
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
                text = text.replace(
                    /ᐊ[0-9,a-f]*ᐅ/, function(x) { return eFB.retrieveFromTag(doc,x); }
                );
                // Write back changes, but ONLY if there are any
                // (since a rewrite will trigger another page load, and thus another parseHTML)
                if (page.innerHTML != text) page.innerHTML = text;
                
                // Now try replace any images with their plaintext
                replaceImages(doc);
                
            }
       }
    }  
}

var replaceImages = function(doc) {
    
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
                getImageSRC(id);
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
                getImageSRC(id);
            }
        }
    );
}

var getImageSRC = function(id) {
    
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
                                    eFB.img_cache[id].docs.forEach( replaceImages );
                                } else {
                                    // Decryption failed
                                    eFB.img_cache[id].status = 1;
                                }
                            }
                        }
                    };
                    SaveImageFromURL( response.source, path, prog_listener );
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
    
}

function SaveImageFromURL(url,path,prog_listener) {
    
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

}



this._loadHandler = function(event) { onPageLoad(event); };
this._loadHandler2 = function(event) { onPageLoad2(event); };

// If not logged in, lookout for succesful authentications
if ( ! eFB.prefs.getBoolPref("loggedIn") ) gBrowser.addEventListener("load", this._loadHandler, true);
// Otherwise lookout for tags to rewrite page
else gBrowser.addEventListener("load", this._loadHandler2, true);


