Drupal.behaviors.islandorasolr = function(context){
  $('.advanced').hide();
  $('#edit-advanced-button').attr('gosh', 'crapola');
  $('#table_selector').toggle(function(){
     $('.advanced').show();
     $('#table_selector').text('Hide advanced columns');
     return false;
  },function(){
     $('.advanced').hide();
     $('#table_selector').text('Show all columns');
     return false;
  } );

    $('#edit-advanced-button').toggle(function(e){
      e.preventDefault();
     $('.advanced').show();
     $('#edit-advanced-button').attr('value', 'Hide advanced columns');
     return false;
  },function(e){
    e.preventDefault();
     $('.advanced').hide();
     $('#edit-advanced-button').attr('value','Show all columns');
     return false;
  } );

};

