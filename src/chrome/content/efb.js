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
            var api_id =    "137547679628995";
            var login_url = "https://graph.facebook.com/oauth/authorize?" +
                            "client_id=" + api_id + "&" +
                            "redirect_uri=http://www.facebook.com/connect/login_success.html&" +
                            "type=user_agent&" +
                            "scope=publish_stream,offline_access,user_about_me,friends_about_me&" +
                            "display=popup";
            window.open( login_url, "fb-login-window", "centerscreen,width=350,height=250,resizable=0");
        } else {
            // And if we are, nothing to do
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
    * Submits a status update, which is also posted to the users wall
    * 
    **/
    submitStatus : function(aEvent) {
        if ( eFB.prefs.getBoolPref("loggedIn") ) {
           
            window.alert( "logged in " + eFB.prefs.getCharPref("token") );
           
        } else {
            window.alert("You aren't logged in to Facebook");
            return;
        }
    }

};

