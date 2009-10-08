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

#include <sqlite3.h>
#include "js_runtime.h"

static JSBool SQLite(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
static void SQLite_Finalize(JSContext *ctx, JSObject *obj);
static  JSBool js_sqlite_exec(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

static JSClass js_SQLiteClass = {
    "SQLite", 
    JSCLASS_HAS_PRIVATE,
    JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub,   JS_ConvertStub,   SQLite_Finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

static JSFunctionSpec sqlite_methods[] = {
  { "exec", js_sqlite_exec, 0,0,0 }
};

JSObject* js_InitSQLiteClass(JSContext *ctx, JSObject *obj) {

    debug("js_InitSQLiteClass\n");
    JSObject *proto;
    
    proto = JS_InitClass(ctx, obj, NULL, &js_SQLiteClass, SQLite, 1, 
            NULL, sqlite_methods, NULL, NULL);
            
    debug("js_InitSQLiteClass finished: %x\n", proto);
    return proto;  
}

static JSBool SQLite(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    sqlite3 *db;
    char *dbname;
    int ret;

    
    js_val_to_string(ctx, argv[0], &dbname);
    ret = sqlite3_open(dbname, &db);
   
    if (ret != SQLITE_OK) {
        fprintf(stderr, "Error opening sql database %s: %s\n", dbname, sqlite3_errmsg(db));
        sqlite3_close(db);
        free(dbname);
        return JS_FALSE;
    }
    free(dbname);
    
    JS_SetPrivate(ctx, obj, db);
    
    return JS_TRUE;
}

static void SQLite_Finalize(JSContext *ctx, JSObject *obj) {
    sqlite3 *db = (sqlite3*)JS_GetPrivate(ctx, obj);
    debug("SQLite_Finalize\n");
    
    sqlite3_close(db);
}

static  JSBool js_sqlite_exec(JSContext *ctx, JSObject *obj, uintN argc, jsval *argv, jsval *rval) {
    sqlite3 *db;
    char *sql, *error;
    JSBool ret = JS_FALSE;
    
    db = (sqlite3*)JS_GetPrivate(ctx, obj);
    
    js_val_to_string(ctx, argv[0], &sql);
    
    debug("Execing sql: %s\n", sql);
    fflush(stderr);
    
    if (sqlite3_exec(db, sql, NULL, NULL, &error) != SQLITE_OK) {
      fprintf(stderr, "Error executing SQL %s: %s\n", sql, error);
      sqlite3_free(error);
    } else {
        ret = JS_TRUE;
    }
    free(sql);
    
    return JS_TRUE;
    
    

}
