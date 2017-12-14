// Remove header and footer.
document.body.innerHTML = document.getElementById("main").innerHTML;
// Remove archive selector.
document.getElementById("archive-selector").remove();
// Remove all non-ios content
var non_ios = document.getElementsByClassName("non-ios");
for (var i = 0; i < non_ios.length; i++) { non_ios[i].remove(); }
// Remove all links.
while (document.links.length) { document.links[0].outerHTML =  document.links[0].innerHTML }
