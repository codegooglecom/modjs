/**
    mod_js - Apache module to run serverside Javascript
    Copyright (C) 2007-2009, Ash Berlin & Tom Insam

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version of the GPL or the Apache License,
    Version 2.0 <http://www.apache.org/licenses/LICENSE-2.0>.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/
#ifndef mod_js_runtime_h
#define mod_js_runtime_h

#include "httpd.h"
#include "http_config.h"
#include "http_protocol.h"
#include "util_script.h"


#define MOD_JS_VERSION "0.1"

#include "jsapi.h"

typedef struct modjsContext {
  JSRuntime *rt;
  JSContext *ctx;
  request_rec *request;
} modjsContext;

modjsContext*  new_context(JSRuntime* rt);
void           free_context(modjsContext* ctx);
JSBool         js_eval(modjsContext *ctx, const char* source, const char* name, jsval*);
JSBool         js_eval_file(modjsContext *ctx, const char *name, jsval*);
void           js_get_exception(modjsContext *ctx, jsval*);
void           js_val_to_string(JSContext *ctx, jsval v, char** string);


#endif
