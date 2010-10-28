encryptedfacebook.onFirefoxLoad = function(event) {
  document.getElementById("contentAreaContextMenu")
          .addEventListener("popupshowing", function (e){ encryptedfacebook.showFirefoxContextMenu(e); }, false);
};

encryptedfacebook.showFirefoxContextMenu = function(event) {
  // show or hide the menuitem based on what the context menu is on
  document.getElementById("context-encryptedfacebook").hidden = gContextMenu.onImage;
};

window.addEventListener("load", encryptedfacebook.onFirefoxLoad, false);
