/**
 * EFB namespace.
 */
if ("undefined" == typeof(EFB)) {
  var EFB = {};
};

/**
 * Controls the browser overlay for the Hello World extension.
 */
EFB.BrowserOverlay = {
  /**
   * Says 'Hello' to the user.
   */
  sayHello : function(aEvent) {
    window.alert('fuck off');
  }
};
