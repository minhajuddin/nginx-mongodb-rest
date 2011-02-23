#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <signal.h>
#include <stdio.h>

#include "mongo-c-driver/src/bson.h"
#include "mongo-c-driver/src/mongo.h"

#define CONTENT_TYPE "application/json"
#define MONGO_HOST "127.0.0.1"
#define MONGO_PORT 27017

static ngx_int_t ngx_http_mongodb_rest_handler(ngx_http_request_t* request);
static char* ngx_http_mongodb_rest(ngx_conf_t* cf, ngx_command_t* command, void* void_conf);
static char* ngx_http_mongodb_rest_merge_loc_conf(ngx_conf_t* cf, void* void_parent, void* void_child);
static void* ngx_http_mongodb_rest_create_loc_conf(ngx_conf_t* conf);
static void get(ngx_str_t* db, ngx_str_t* collection, char* id, ngx_buf_t* b);
static void ngx_http_mongodb_rest_exit_worker(ngx_cycle_t* cycle);

static ngx_int_t ngx_http_mongodb_rest_init_worker(ngx_cycle_t* cycle);

//connection shared by requests for a single worker
mongo_connection cached_connection[1];

typedef struct {
  ngx_str_t db;
  ngx_str_t collection;
} ngx_http_mongodb_rest_loc_conf_t;


static ngx_command_t ngx_http_mongodb_rest_commands[] = {
  {
    ngx_string("mongodb_rest"),
    NGX_HTTP_LOC_CONF | NGX_CONF_1MORE,
    ngx_http_mongodb_rest,
    NGX_HTTP_LOC_CONF_OFFSET,
    0,
    NULL
  },
  ngx_null_command
};


static ngx_http_module_t ngx_http_mongodb_rest_module_ctx = {
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  NULL,
  ngx_http_mongodb_rest_create_loc_conf,
  ngx_http_mongodb_rest_merge_loc_conf
};


ngx_module_t ngx_http_mongodb_rest_module = {
  NGX_MODULE_V1,
  &ngx_http_mongodb_rest_module_ctx,
  ngx_http_mongodb_rest_commands,
  NGX_HTTP_MODULE,
  NULL,
  NULL,
  ngx_http_mongodb_rest_init_worker,
  NULL,
  NULL,
  ngx_http_mongodb_rest_exit_worker,
  NULL,
  NGX_MODULE_V1_PADDING
};


static char* ngx_http_mongodb_rest(ngx_conf_t* cf, ngx_command_t* command, void* void_conf){
  ngx_http_mongodb_rest_loc_conf_t *mr_conf = void_conf;
  ngx_http_core_loc_conf_t* core_conf;
  ngx_str_t *value;

  core_conf = ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);
  core_conf->handler = ngx_http_mongodb_rest_handler;

  value = cf->args->elts;

  mr_conf->db = value[1];
  mr_conf->collection = value[2];

  return NGX_CONF_OK;
}


static ngx_int_t ngx_http_mongodb_rest_handler(ngx_http_request_t* r){
  ngx_buf_t *b;
  ngx_chain_t out;
  ngx_http_mongodb_rest_loc_conf_t *mr_conf;

  mr_conf = ngx_http_get_module_loc_conf(r, ngx_http_mongodb_rest_module);

  b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));

  if (b == NULL) {
    ngx_log_error(NGX_LOG_ERR, r->connection->log, 0, 
        "Failed to allocate response buffer.");
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  /* set the headers of the response */
  r->headers_out.status = NGX_HTTP_OK;
  r->headers_out.content_type.len = sizeof(CONTENT_TYPE) - 1;
  r->headers_out.content_type.data = (u_char *) CONTENT_TYPE;
  ngx_http_send_header(r);

  /* get the result and write it to the buffer */
  get(&mr_conf->db, &mr_conf->collection, "12", b);

  b->memory = 1; /* content is in read-only memory */
  /* (i.e., filters should copy it rather than rewrite in place) */
  b->last_buf = 1;  /* there will be no more buffers in the request */

  out.buf = b;
  out.next = NULL;

  return ngx_http_output_filter(r, &out);
}

static void* ngx_http_mongodb_rest_create_loc_conf(ngx_conf_t* conf){
  ngx_http_mongodb_rest_loc_conf_t* mr_conf;

  mr_conf = ngx_pcalloc(conf->pool, sizeof(ngx_http_mongodb_rest_loc_conf_t));
  return mr_conf;
}


static char* ngx_http_mongodb_rest_merge_loc_conf(ngx_conf_t* cf, void* void_parent, void* void_child){
  ngx_http_mongodb_rest_loc_conf_t *parent = void_parent;
  ngx_http_mongodb_rest_loc_conf_t *child = void_child;

  ngx_conf_merge_str_value(child->db, parent->db, NULL);
  ngx_conf_merge_str_value(child->collection, parent->collection, NULL);

  return NGX_CONF_OK;
}

/* mongodb functions */
static ngx_int_t log_mongo_error(ngx_log_t *log, mongo_conn_return status);
/*int get_mongo_connection(ngx_log_t *log);*/

static void get(ngx_str_t *db, ngx_str_t *collection, char* id, ngx_buf_t *b){
  bson_buffer bb;
  bson obj;
  bson cond;
  u_char result[1000];
  char ns[1000];

  //get the query
  bson_buffer_init(&bb);
  bson_append_string(&bb, "_id", id);
  bson_from_buffer(&cond, &bb);

  sprintf(ns, "%s.%s", db->data, collection->data);

  if(!mongo_find_one(cached_connection, ns, &cond, 0, &obj)){
    strcpy((char *)result, "{'error':'record not found'}");
  } else {
    strcpy((char *)result, "{'success':'record found'}");
  }

  b->pos = result; /* address of the first position of the data */
  b->last = result + strlen((char *)result);  /* address of the last position of the data */
  // destroy cond bb and other stuff
}


static ngx_int_t ngx_http_mongodb_rest_init_worker(ngx_cycle_t* cycle) {
  mongo_connection_options opts[1];
  mongo_conn_return status;

  ngx_log_error(NGX_LOG_ERR, cycle->log, 0,"WORKER INIT");

  strcpy( opts->host , MONGO_HOST);
  opts->port = MONGO_PORT;

  status = mongo_connect( cached_connection, opts );

  if (status != mongo_conn_success){
    return log_mongo_error(cycle->log, status);
  }

  return NGX_OK;
}


static void ngx_http_mongodb_rest_exit_worker(ngx_cycle_t* cycle) {
  mongo_disconnect(cached_connection);
  mongo_destroy(cached_connection);
}


/*
int get_mongo_connection(ngx_log_t *log){
  mongo_conn_return status;

  if(cached_connection->connected) { 
    ngx_msleep(500);
    status = mongo_reconnect(cached_connection);
  }

  if (status != mongo_conn_success){
    return log_mongo_error(log, status);
  }

  return NGX_OK;
}
*/



static ngx_int_t log_mongo_error(ngx_log_t *log, mongo_conn_return status){

  switch (status) {
    case mongo_conn_success:
      return NGX_OK;
    case mongo_conn_bad_arg:
      ngx_log_error(NGX_LOG_ERR, log, 0,
          "Mongo Exception: Bad Arguments");
      return NGX_ERROR;
    case mongo_conn_no_socket:
      ngx_log_error(NGX_LOG_ERR, log, 0,
          "Mongo Exception: No Socket");
      return NGX_ERROR;
    case mongo_conn_fail:
      ngx_log_error(NGX_LOG_ERR, log, 0,  "Mongo Exception: Connection Failure 127.0.0.1:27017;");
      return NGX_ERROR;
    case mongo_conn_not_master:
      ngx_log_error(NGX_LOG_ERR, log, 0,
          "Mongo Exception: Not Master");
      return NGX_ERROR;
    default:
      ngx_log_error(NGX_LOG_ERR, log, 0,
          "Mongo Exception: Unknown Error");
      return NGX_ERROR;
  }
}


