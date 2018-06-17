#include <ngx_config.h>
#include <ngx_http.h>


static ngx_int_t ngx_http_upstream_tcpinfo_add_variables(ngx_conf_t *cf);

static ngx_int_t ngx_http_upstream_tcpinfo_variable(ngx_http_request_t *r,
    ngx_http_variable_value_t *v, uintptr_t data);


static ngx_http_variable_t  ngx_http_upstream_tcpinfo_vars[] = {

    { ngx_string("upstream_tcpinfo_rtt"), NULL, ngx_http_upstream_tcpinfo_variable,
      0, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_tcpinfo_rttvar"), NULL, ngx_http_upstream_tcpinfo_variable,
      1, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_tcpinfo_snd_cwnd"), NULL, ngx_http_upstream_tcpinfo_variable,
      2, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    { ngx_string("upstream_tcpinfo_rcv_space"), NULL, ngx_http_upstream_tcpinfo_variable,
      3, NGX_HTTP_VAR_NOCACHEABLE, 0 },

    ngx_http_null_variable
};


static ngx_http_module_t ngx_http_upstream_tcpinfo_module_ctx = {
    ngx_http_upstream_tcpinfo_add_variables, /* preconfiguration */
    NULL,                                    /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    NULL,                                    /* create server configuration */
    NULL,                                    /* merge server configuration */

    NULL,                                    /* create location configuration */
    NULL                                     /* merge location configuration */
};


ngx_module_t  ngx_http_upstream_tcpinfo_module = {
    NGX_MODULE_V1,
    &ngx_http_upstream_tcpinfo_module_ctx, /* module context */
    NULL                      ,            /* module directives */
    NGX_HTTP_MODULE,                       /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};


static ngx_int_t
ngx_http_upstream_tcpinfo_add_variables(ngx_conf_t *cf)
{
    ngx_http_variable_t *var, *v;

    for (v = ngx_http_upstream_tcpinfo_vars; v->name.len; v++) {
        var = ngx_http_add_variable(cf, &v->name, v->flags);
        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

static ngx_int_t
ngx_http_upstream_tcpinfo_variable(ngx_http_request_t *r, ngx_http_variable_value_t *v,
    uintptr_t data)
{
    struct tcp_info  ti;
    socklen_t        len;
    uint32_t         value;

    ngx_socket_t     sock;

    ngx_log_debug0(NGX_LOG_DEBUG, ngx_cycle->log, 0,
            "upstream_tcpinfo: entry");

    if (r->upstream == NULL ||
        r->upstream->peer.connection == NULL) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0,
            "upstream_tcpinfo: no upstream");
        v->not_found = 1;
        return NGX_OK;
    }

    sock = r->upstream->peer.connection->fd;

    len = sizeof(struct tcp_info);
    if (getsockopt(sock, IPPROTO_TCP, TCP_INFO, &ti, &len) == -1) {
        ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0,
            "upstream_tcpinfo: no tcpinfo");
        v->not_found = 1;
        return NGX_OK;
    }

    ngx_log_debug0(NGX_LOG_DEBUG_HTTP, ngx_cycle->log, 0,
        "upstream_tcpinfo: got it");

    v->data = ngx_pnalloc(r->pool, NGX_INT32_LEN);
    if (v->data == NULL) {
        return NGX_ERROR;
    }

    switch (data) {
    case 0:
        value = ti.tcpi_rtt;
        break;

    case 1:
        value = ti.tcpi_rttvar;
        break;

    case 2:
        value = ti.tcpi_snd_cwnd;
        break;

    case 3:
        value = ti.tcpi_rcv_space;
        break;

    /* suppress warning */
    default:
        value = 0;
        break;
    }

    v->len = ngx_sprintf(v->data, "%uD", value) - v->data;
    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    return NGX_OK;
}
