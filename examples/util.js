
var params = {};
var tree;

(function() {
  if (request.env.QUERY_STRING) {
    var atoms = request.env.QUERY_STRING.split("&");
    for each (var i in atoms) {
      var [a,b] = i.split("=");
      params[a] = b;
    }
  }

  tree = function tree(obj, name) {
    for (var prop in obj) {
      if (obj[prop] == tree) continue;
      print( name + "." + prop + " = " + obj[prop] +"\n" );
      if (obj[prop] instanceof Object) {
        tree( obj[prop], name + "." + prop );
      }
    }
  }

})();
