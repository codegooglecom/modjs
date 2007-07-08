include('/Users/tomi/Sites/scripts/util.js');

print("<html><head>");

print("<title>Page title here</title>");

print("</head>");

print("<body>");

print("<pre>");
tree( params, "params");
print("</pre>");

print(<form method="get">
<input name="value" />
<input type="submit" />
</form>);

print("<pre>\n");
tree( this, "this" );

print("</body></html");


