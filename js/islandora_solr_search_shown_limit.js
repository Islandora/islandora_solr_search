$(document).ready(function() {
  
  // show more  
  $(".shown-toggle").click(function() {
  
    $(this).siblings(".hidden, .toggle-wrapper").toggleClass('hidden');
    $(this).toggleClass('hidden');

    return false;
   });

});