/**
 * EncryptedFacebookChrome namespace.
 */
if ("undefined" == typeof(EncryptedFacebookChrome)) {
  var EncryptedFacebookChrome = {};
};

/**
 * Controls the browser overlay for the Hello World extension.
 */
EncryptedFacebookChrome.BrowserOverlay = {
  /**
   * Says 'Hello' to the user.
   */
  sayHello : function(aEvent) {

    var result = window.prompt("Enter submission text");
    
    window.alert( result );

  }
};
