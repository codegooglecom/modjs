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

#include "jsapi.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdarg.h>

#include "js_runtime.h"

void printError(JSContext *cx, const char *message, JSErrorReport *report);

void debug(char * fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

static JSRuntime* init_runtime() {
  JSRuntime* runtime;
  runtime = JS_NewRuntime( 1024 * 1024 );
  return runtime;
}

/* Global class, does nothing */
static JSClass global_class = {
    "global", 0,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

modjsContext* new_context(JSRuntime* rt) {
  modjsContext *ctx;
  JSObject* obj;

  ctx = malloc(sizeof(modjsContext));
  if (ctx == NULL) {
    fprintf(stderr, "Whoop! Whoop! No memory left!");
    abort();
  }

  if (rt == NULL) {
    ctx->rt = init_runtime();
  } else {
    ctx->rt = rt;
  }
  
  ctx->ctx = JS_NewContext(ctx->rt, 8192);
  if (ctx->ctx == NULL) {
    fprintf(stderr, "Unable to create JS context!");
    return NULL;
  }

  JS_SetVersion(ctx->ctx, JSVERSION_1_7);
  JS_SetOptions(ctx->ctx, JSOPTION_DONT_REPORT_UNCAUGHT);

  obj = JS_NewObject(ctx->ctx, &global_class, NULL, NULL);
  if (JS_InitStandardClasses(ctx->ctx, obj) == JS_FALSE) {
    debug("failed to initialize standard classes\n");
    free_context(ctx);
    return NULL;
  }
  
  //TODO: Make this hookable!
  //extern JSObject* js_InitSQLiteClass(JSContext *ctx, JSObject *obj);
  //js_InitSQLiteClass(ctx->ctx, obj);

  // Store our structure in the ctx so we can go back and forth as needed
  JS_SetContextPrivate(ctx->ctx, (void*) ctx);

  return ctx;
}

void free_context(modjsContext* ctx) {
  // TODO: Do we need to keep track of any bound functions? Probally

  JS_DestroyContext(ctx->ctx);

  free(ctx);
}

JSBool js_eval(modjsContext *ctx, const char * source, const char * name, jsval *rval) {
  JSObject *gobj = JS_GetGlobalObject(ctx->ctx);
  return JS_EvaluateScript(ctx->ctx, gobj, source, strlen(source), name, 1, rval);
}

JSBool set_exception(modjsContext *ctx, char *fmt, ...) {
    char *string;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&string, fmt, ap);
    va_end(ap);
    jsval j = STRING_TO_JSVAL( JS_NewStringCopyN( ctx->ctx, string, strlen( string ) ) );
    free(string);
    JS_SetPendingException( ctx->ctx, j );
    return JS_FALSE;
}

JSBool js_eval_file(modjsContext *ctx, const char* name, jsval* rval) {
  FILE *fh;
  struct stat fsize;

  fh = fopen(name, "r");

  if (fh == NULL) {
    return set_exception( ctx, "Error opening %s: %s\n", name, strerror( errno ));
  }

  if (fstat(fileno(fh), &fsize) == -1) {
    return set_exception( ctx, "Error statting %s: %s\n", name, strerror( errno ));
  }

  char * buff = malloc(fsize.st_size + 1);
  if (buff == NULL) {
    fprintf(stderr, "Error allocating bufer for reading JS file!\n");
    fclose(fh);
    abort();
  }

  fread(buff, fsize.st_size, 1, fh);
  if (ferror(fh)) {
    set_exception( ctx, "Error reading data from %s: %s\n", name, strerror(errno));
    free(buff);
    fclose(fh);
    return JS_FALSE;
  }

  fclose(fh);
  
  buff[ fsize.st_size ] = 0;

  JSBool ok = js_eval(ctx, buff, name, rval);
  free(buff);

  return ok;
}

void js_get_exception(modjsContext *ctx, jsval *rval) {

  if (JS_IsExceptionPending( ctx->ctx ) == JS_FALSE) {
    debug("no exception pending!\n");
    *rval = JSVAL_NULL;
  }
  
  JS_GetPendingException(ctx->ctx, rval);
  JS_ClearPendingException(ctx->ctx);
}


/* Converts a JavaScript value to a string. CALLER MUST FREE STRING */
void js_val_to_string(JSContext *ctx, jsval v, char** string) {
    JSString *str = JS_ValueToString( ctx, v );
    *string = (char*)JS_smprintf("%hs", JS_GetStringChars(str));
}



void js_define_function(modjsContext *ctx, const char *name, JSNative call) {
    if (JS_DefineFunction(ctx->ctx, JS_GetGlobalObject(ctx->ctx), name, call, 0, 0) == JS_FALSE) {
        debug("Failed to define function");
    }
}
