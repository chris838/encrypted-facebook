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
                
                
                // Try and replace any images
                // First try to get all facebook user images on the page
                // TODO - get i tags aswell
                var list = doc.getElementById("content").getElementsByTagName('img'); 
                                
                // Try and get rid of non-facebook graph api image objects
                var ar = [];
                for (var i=0; i<list.length; i++) {
                    if ((( list.item(i).src.indexOf('photos') != -1)
                         || ( list.item(i).src.indexOf('profile') != -1))
                        && ( list.item(i).src.indexOf('fbcdn.net') != -1))
                        ar.push( list.item(i) );
                }
                
                // For each image, get the filename and try each id with the graph api
                // to check if it is an image
                ar.forEach( function(x) {
                        var y = x.src.split('/');
                        var z = y[ y.length -1 ];
                        window.alert(z);
                    }
                );
                
                
                // We maintain a list of image IDs that have been processed. For each ID there are
                // three possible statuses.
                
                    // 0: Decryption tried but failed - object is not encrypted or we don't have the key.
                    // Do nothing.
                    
                    // 1: Object is encrypted, plaintext exists in cache.
                    // Replace image on page with cached plaintext (if we haven't already).
                    
                    // 2: Object is being processed (may or may not be decryptable).
                    // Add doc to list of docs that need updating on completion (if we haven't already).
                    // Replace image on page with loading image (if we haven't already).
                    
                    // 3: Object does not exist in cache.
                    // Create pending cache entry (include reference to this doc) and initiate request.
                    // Replace image on page with loading image.
                
            }
       }
    }  
}

this._loadHandler = function(event) { onPageLoad(event); };
this._loadHandler2 = function(event) { onPageLoad2(event); };

// If not logged in, lookout for succesful authentications
if ( ! eFB.prefs.getBoolPref("loggedIn") ) gBrowser.addEventListener("load", this._loadHandler, true);
// Otherwise lookout for tags to rewrite page
else gBrowser.addEventListener("load", this._loadHandler2, true);


