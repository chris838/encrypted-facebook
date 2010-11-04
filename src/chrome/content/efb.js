/**
 * Main Encrypted Facebook script
 *
 */

var efbLib = {
    
    submitStatus : function() {
        
        var x = document.getElementById("efb-input").value;
        
        fbLib.SetFacebookStatus(x);
        
    }

}