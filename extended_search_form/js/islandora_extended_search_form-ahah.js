$(document).ready(function() {
  $("body").ajaxComplete(function(event, request, settings) {
    var response = eval("(" + request.responseText + ")");
    jQuery.extend(Drupal.settings, response.settings);
    Drupal.attachBehaviors();
  });
});
