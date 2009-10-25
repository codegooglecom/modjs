/**
    mod_js - Apache module to run serverside Javascript
    Copyright (C) 2007, Ash Berlin & Tom Insam

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "util_script.h"

#include <stdio.h>

#include "js_runtime.h"
#include "js_cgi.h"


JSBool cgi_print(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    modjsContext *mjs = (modjsContext*)JS_GetContextPrivate(cx);
    if (argc > 0) {
        char *string;
        if (js_val_to_string( cx, argv[0], &string ) == 0) {
	  ap_rprintf(mjs->request, "%s", string);
	  free(string);
	  return JS_TRUE;
	}
    }
    return JS_FALSE;
}

JSBool cgi_include(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    modjsContext *mjs = (modjsContext*)JS_GetContextPrivate(cx);
    if (argc > 0) {
        char *string;
        if (js_val_to_string( cx, argv[0], &string ) == 0) {
	  JSBool ok = js_eval_file( mjs, string, rval );
	  free(string);
	  return ok;
	}
    }
    return JS_FALSE;
}




void js_import_functions( modjsContext *mjs ) {
    js_define_function( mjs, "print", cgi_print );
    js_define_function( mjs, "include", cgi_include );
}

