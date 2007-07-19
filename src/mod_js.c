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

static int mod_js_init_handler(apr_pool_t *p, apr_pool_t *plog, apr_pool_t *ptemp, server_rec *s)
{
    ap_add_version_component(p, "mod_js/" MOD_JS_VERSION );
    return OK;
}

static int mod_js_child_init(apr_pool_t *p, server_rec *s)
{
    return OK;
}

static int mod_js_handler(request_rec *r)
{
    int i;

    modjsContext* mjs;

    /* things going IN */
    const apr_array_header_t *arr;
    const apr_table_entry_t *elts;
    char *request_content; /* the full request sent to the server */

    /* things coming OUT */
    jsval rval;

    if (!r->handler ||  strcmp(r->handler, "js_script"))
        return DECLINED;

    /* create new context */
    mjs = new_context( NULL );
    if (mjs == NULL) {
        /* TODO - return 500 */
        fprintf(stderr, "Unable to create JS context!\n");
        return HTTP_INTERNAL_SERVER_ERROR;
    }
    
    mjs->request = r;
    
    js_import_functions( mjs );

    /* read in the request content from the client */
    /* TODO - this slurps the whole thing at once. Not very nice. */
    if (ap_setup_client_block(r, REQUEST_CHUNKED_DECHUNK) == OK) {
        char *data;
        int read;
        request_content = malloc( r->remaining + 1 );
        for (i = 0; i <= r->remaining; i++)
            request_content[i] = 0;

        data = malloc( 1024 ); /* made up size. */
        while ( (read = ap_get_client_block(r, data, 1024)) ) {
            strncat(request_content, data, read);
        }
        free(data);
    }

    /* initialize the CGI environment */
    ap_add_common_vars(r);
    ap_add_cgi_vars(r);

    /* extract the CGI environment */
    arr = apr_table_elts(r->subprocess_env);
    elts = (const apr_table_entry_t*)arr->elts;

    JSObject *request = JS_DefineObject( mjs->ctx,
      JS_GetGlobalObject( mjs->ctx ), "request", NULL, NULL, JSPROP_ENUMERATE );

    JSObject *environment = JS_DefineObject( mjs->ctx, request, "env", NULL, NULL, JSPROP_ENUMERATE );

    for (i = 0; i < arr->nelts; ++i) {
        if (!elts[i].key || !elts[i].val) continue;
        jsval val = STRING_TO_JSVAL( JS_NewStringCopyN( mjs->ctx, elts[i].val, strlen( elts[i].val ) ) );
        JS_DefineProperty( mjs->ctx, environment, elts[i].key, val, NULL, NULL, JSPROP_ENUMERATE );
    }

    jsval post_data = STRING_TO_JSVAL( JS_NewStringCopyN( mjs->ctx, request_content, strlen( request_content ) ) );
    JS_DefineProperty( mjs->ctx, request, "post_data", post_data, NULL, NULL, JSPROP_ENUMERATE );

    /* set apache headers */
    /* TODO - let the script do this. */
    r->content_type = "text/html";

    if (js_eval_file( mjs, r->filename, &rval ) == JS_FALSE) {
        char * content;
        js_get_exception( mjs, &rval );
        js_val_to_string( mjs->ctx, rval, &content );
        /* TODO - send this as error */
        if (!r->header_only)
            ap_rprintf(r, "%s", content );
        free(content);
    }

    /* done with JS context now. */
    free_context( mjs );

    /* we've processed, we don't need the POST data any more */
    if (request_content) free(request_content);

    return OK;
}

/****************************************************************
 * apache stuff
 ****************************************************************/

static void mod_js_register_hooks(apr_pool_t *p ) {
    ap_hook_handler(mod_js_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_post_config(mod_js_init_handler, NULL, NULL, APR_HOOK_MIDDLE);
    ap_hook_child_init(mod_js_child_init, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA js_module = {
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    mod_js_register_hooks,
};
