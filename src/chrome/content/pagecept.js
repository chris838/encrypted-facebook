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
        }        
                
        // Check the URL for the dummy Facebook succesful login page
        var url = doc.defaultView.location.href;
        var rx=/facebook\.com\//; // TODO - watch out for malicious code injections
        if (rx.test(url)) {
            
            window.alert( doc.body.innerHTML );
            
        }  
    }  
}

this._loadHandler = function(event) { onPageLoad(event); };
this._loadHandler2 = function(event) { onPageLoad2(event); };

// If not logged in, lookout for succesful authentications
if ( ! eFB.prefs.getBoolPref("loggedIn") ) gBrowser.addEventListener("load", this._loadHandler, true);
// Otherwise lookout for tags to rewrite page
else gBrowser.addEventListener("load", this._loadHandler2, true);


