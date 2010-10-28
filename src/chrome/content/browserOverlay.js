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
    let stringBundle = document.getElementById("encryptedfacebook-string-bundle");
    let message = stringBundle.getString("encryptedfacebook.greeting.label");

    window.alert(message);
  }
};
