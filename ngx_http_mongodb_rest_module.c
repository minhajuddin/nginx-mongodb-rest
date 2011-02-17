#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include <signal.h>
#include <stdio.h>

static ngx_int_t ngx_http_mongodb_rest_handler(ngx_http_request_t* request);
static char* ngx_http_mongodb_rest(ngx_conf_t* cf, ngx_command_t* command, void* void_conf);
static char* ngx_http_mongodb_rest_merge_loc_conf(ngx_conf_t* cf, void* void_parent, void* void_child);
static void* ngx_http_mongodb_rest_create_loc_conf(ngx_conf_t* conf);

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
  NULL,
  NULL,
  NULL,
  NULL,
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


  ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "db is %s", mr_conf->db.data);
  ngx_conf_log_error(NGX_LOG_EMERG, cf, 0, "collection is %s", mr_conf->collection.data);

  return NGX_CONF_ERROR;
}


static ngx_int_t ngx_http_mongodb_rest_handler(ngx_http_request_t* request){
  return NGX_OK;
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
