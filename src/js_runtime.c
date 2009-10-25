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

modjsContext* new_context(gpsee_interpreter_t *jsi) {
  modjsContext *ctx;
  JSObject* obj;

  ctx = malloc(sizeof(modjsContext));
  if (ctx == NULL) {
    fprintf(stderr, "Whoop! Whoop! No memory left!");
    abort();
  }

  if (jsi == NULL) {
    ctx->jsi = gpsee_createInterpreter(NULL, NULL);
  } else {
    ctx->jsi = jsi;
  }
  
  if (ctx->jsi == NULL || ctx->jsi->cx == NULL) {
    fprintf(stderr, "Unable to create JS context!");
    return NULL;
  }

  gpsee_verbosity(2);

  //TODO: Make this hookable!
  //extern JSObject* js_InitSQLiteClass(JSContext *ctx, JSObject *obj);
  //js_InitSQLiteClass(ctx->ctx, obj);

  // Store our structure in the ctx so we can go back and forth as needed
  JS_SetContextPrivate(ctx->jsi->cx, (void*) ctx);

  return ctx;
}

void free_context(modjsContext* ctx) {
  // TODO: Do we need to keep track of any bound functions? Probally

  gpsee_destroyInterpreter(ctx->jsi);

  free(ctx);
}

JSBool js_eval(modjsContext *ctx, const char * source, const char * name, jsval *rval) {
  return JS_EvaluateScript(ctx->jsi->cx, ctx->jsi->globalObj, source, strlen(source), name, 1, rval);
}

JSBool set_exception(modjsContext *ctx, char *fmt, ...) {
    char *string;
    va_list ap;
    va_start(ap, fmt);
    vasprintf(&string, fmt, ap);
    va_end(ap);
    jsval j = STRING_TO_JSVAL( JS_NewStringCopyN( ctx->jsi->cx, string, strlen( string ) ) );
    free(string);
    JS_SetPendingException( ctx->jsi->cx, j );
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

  if (JS_IsExceptionPending( ctx->jsi->cx ) == JS_FALSE) {
    debug("no exception pending!\n");
    *rval = JSVAL_NULL;
  }
  
  JS_GetPendingException(ctx->jsi->cx, rval);
  JS_ClearPendingException(ctx->jsi->cx);
}


/* Converts a JavaScript value to a string. CALLER MUST FREE STRING */
int js_val_to_string(JSContext *ctx, jsval v, char** string) {
    JSString *str = JS_ValueToString( ctx, v );
    if (str)
      *string = (char*)JS_smprintf("%hs", JS_GetStringChars(str));
    else
      return 1;

    return 0;
}



void js_define_function(modjsContext *ctx, const char *name, JSNative call) {
    if (JS_DefineFunction(ctx->jsi->cx, ctx->jsi->globalObj, name, call, 0, 0) == JS_FALSE) {
        debug("Failed to define function");
    }
}
