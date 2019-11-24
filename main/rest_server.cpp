/* HTTP Restful API Server

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include <fcntl.h>
#include "esp_http_server.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs.h"
#include "cJSON.h"

#include <FastLED.h>
#include "alx_types.h"

extern lc_config_t lc_config;
extern int num_color_palettes;
extern int color_palette_size[];
extern CRGB color_palette[][6];

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define SCRATCH_BUFSIZE (10240)

typedef struct rest_server_context {
    char base_path[ESP_VFS_PATH_MAX + 1];
    char scratch[SCRATCH_BUFSIZE];
} rest_server_context_t;

#define CHECK_FILE_EXTENSION(filename, ext) (strcasecmp(&filename[strlen(filename) - strlen(ext)], ext) == 0)

/* Set HTTP response content type according to file extension */
static esp_err_t set_content_type_from_file(httpd_req_t *req, const char *filepath)
{
    const char *type = "text/plain";
    if (CHECK_FILE_EXTENSION(filepath, ".html")) {
        type = "text/html";
    } else if (CHECK_FILE_EXTENSION(filepath, ".js")) {
        type = "application/javascript";
    } else if (CHECK_FILE_EXTENSION(filepath, ".css")) {
        type = "text/css";
    } else if (CHECK_FILE_EXTENSION(filepath, ".png")) {
        type = "image/png";
    } else if (CHECK_FILE_EXTENSION(filepath, ".ico")) {
        type = "image/x-icon";
    } else if (CHECK_FILE_EXTENSION(filepath, ".svg")) {
        type = "text/xml";
    }
    return httpd_resp_set_type(req, type);
}


/* Send HTTP response with the contents of the requested file */
static esp_err_t rest_common_get_handler(httpd_req_t *req)
{
    char filepath[FILE_PATH_MAX];

    rest_server_context_t *rest_context = (rest_server_context_t *)req->user_ctx;
    strlcpy(filepath, rest_context->base_path, sizeof(filepath));
    if (req->uri[strlen(req->uri) - 1] == '/') {
        strlcat(filepath, "/index.html", sizeof(filepath));
    } else {
        strlcat(filepath, req->uri, sizeof(filepath));
    }
    int fd = open(filepath, O_RDONLY, 0);
    if (fd == -1) {
        ESP_LOGE(LOGTAG_REST, "Failed to open file : %s", filepath);
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        return ESP_FAIL;
    }

    set_content_type_from_file(req, filepath);

    char *chunk = rest_context->scratch;
    ssize_t read_bytes;
    do {
        /* Read file in chunks into the scratch buffer */
        read_bytes = read(fd, chunk, SCRATCH_BUFSIZE);
        if (read_bytes == -1) {
            ESP_LOGE(LOGTAG_REST, "Failed to read file : %s", filepath);
        } else if (read_bytes > 0) {
            /* Send the buffer contents as HTTP response chunk */
            if (httpd_resp_send_chunk(req, chunk, read_bytes) != ESP_OK) {
                close(fd);
                ESP_LOGE(LOGTAG_REST, "File sending failed!");
                /* Abort sending file */
                httpd_resp_sendstr_chunk(req, NULL);
                /* Respond with 500 Internal Server Error */
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (read_bytes > 0);
    /* Close file after sending complete */
    close(fd);
    ESP_LOGI(LOGTAG_REST, "File sending complete");
    /* Respond with an empty chunk to signal HTTP response completion */
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

/* handler for lc config */
static esp_err_t lc_config_post_handler(httpd_req_t *req)
{
    int total_len = req->content_len;
    int cur_len = 0;
    char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
    int received = 0;
    if (total_len >= SCRATCH_BUFSIZE) {
        /* Respond with 500 Internal Server Error */
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
        return ESP_FAIL;
    }
    while (cur_len < total_len) {
        received = httpd_req_recv(req, buf + cur_len, total_len);
        if (received <= 0) {
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
            return ESP_FAIL;
        }
        cur_len += received;
    }
    buf[total_len] = '\0';

    cJSON *root = cJSON_Parse(buf);
    int mode = cJSON_GetObjectItem(root, "mode")->valueint;
    int brightness = cJSON_GetObjectItem(root, "brightness")->valueint;
    int color_id = cJSON_GetObjectItem(root, "color_id")->valueint;
    int cs = cJSON_GetObjectItem(root, "color_palette")->valueint;
    int remote_onoff = cJSON_GetObjectItem(root, "remote_onoff")->valueint;
    if (mode < 0 || mode >= LC_LAST_MODE || cs > num_color_palettes || color_id > color_palette_size[cs] || brightness > lc_config.max_bright || (remote_onoff > 1 || remote_onoff < 0)) {
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid light controller config");
            return ESP_FAIL;
    }

    lc_config.set_mode = (lc_mode)mode;
    lc_config.set_bright = brightness;
    lc_config.color = color_id;
    lc_config.color_palette = cs;
    lc_config.remote_onoff = remote_onoff;
    ESP_LOGI(LOGTAG_REST, "LC config: mode = %d, brightness = %d, color_id = %d, color_palette = %d, remote_onoff = %d", mode, brightness, color_id, cs, remote_onoff);
    cJSON_Delete(root);
    httpd_resp_sendstr(req, "Post control value successfully");

    /* store new values to NVS */
    nvs_update_config("alx.lcc", "mode", mode);
    nvs_update_config("alx.lcc", "set_bright", brightness);
    nvs_update_config("alx.lcc", "color", color_id);
    nvs_update_config("alx.lcc", "color_palette", cs);
    return ESP_OK;
}


/* handler for getting colors */
static esp_err_t lc_cols_get_handler(httpd_req_t *req)
{
	char hexcol[8];
	int cs, i;

	httpd_resp_set_type(req, "application/json");
	cJSON *root = cJSON_CreateObject();

	cJSON *cols = cJSON_CreateArray();
	if (NULL == cols) {
		return ESP_FAIL;
	}

	cJSON_AddItemToObject(root, "cols", cols);

	for (cs = 0; cs < num_color_palettes; ++cs) {
		cJSON_AddItemToArray(cols, cJSON_CreateArray());
		for (i = 0; i < color_palette_size[cs]; ++i) {
			sprintf(hexcol, "#%02X%02X%02X", color_palette[cs][i].r, color_palette[cs][i].g, color_palette[cs][i].b);
			cJSON_AddItemToArray(cJSON_GetArrayItem(cols, cs), cJSON_CreateString(hexcol));
		}
	}

	const char *cols_json = cJSON_Print(root);
	httpd_resp_sendstr(req, cols_json);
	free((void *)cols_json);
	cJSON_Delete(root);
	return ESP_OK;
}


/* handler for getting light controller config */
static esp_err_t lc_config_get_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "application/json");
    cJSON *root = cJSON_CreateObject();

    cJSON_AddNumberToObject(root, "mode", lc_config.set_mode);
    cJSON_AddNumberToObject(root, "brightness", lc_config.set_bright);
    cJSON_AddNumberToObject(root, "color", lc_config.color);
    cJSON_AddNumberToObject(root, "color_palette", lc_config.color_palette);
    cJSON_AddNumberToObject(root, "remote_onoff", lc_config.remote_onoff);
    cJSON_AddNumberToObject(root, "num_palettes", num_color_palettes);

    const char *conf_json = cJSON_Print(root);
    httpd_resp_sendstr(req, conf_json);
    free((void *)conf_json);
    cJSON_Delete(root);
    return ESP_OK;
}


/* handler for updating color definition */
static esp_err_t lc_coldef_post_handler(httpd_req_t *req)
{
	int col_r, col_g, col_b;
	int total_len = req->content_len;
	int cur_len = 0;
	char *buf = ((rest_server_context_t *)(req->user_ctx))->scratch;
	int received = 0;
	if (total_len >= SCRATCH_BUFSIZE) {
		/* Respond with 500 Internal Server Error */
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "content too long");
		return ESP_FAIL;
	}
	while (cur_len < total_len) {
		received = httpd_req_recv(req, buf + cur_len, total_len);
		if (received <= 0) {
			/* Respond with 500 Internal Server Error */
			httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to post control value");
			return ESP_FAIL;
		}
		cur_len += received;
	}
	buf[total_len] = '\0';

	cJSON *root = cJSON_Parse(buf);
	int color_id = cJSON_GetObjectItem(root, "color_id")->valueint;
	int cs = cJSON_GetObjectItem(root, "color_palette")->valueint;
	char *hexcolor = cJSON_GetObjectItem(root, "hexvalue")->valuestring;
	if (3 != sscanf(hexcolor, "#%2X%2X%2X", &col_r, &col_g, &col_b) || cs > num_color_palettes || color_id > color_palette_size[cs]) {
		httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid color definition");
		return ESP_FAIL;
	}

	color_palette[cs][color_id].r = col_r;
	color_palette[cs][color_id].g = col_g;
	color_palette[cs][color_id].b = col_b;
	ESP_LOGI(LOGTAG_REST, "LC coldef: color = %d,%d,%d, color_id = %d, color_palette = %d", col_r, col_g, col_b, color_id, cs);
	cJSON_Delete(root);
	httpd_resp_sendstr(req, "Post control value successfully");

	nvs_update_coldef("alx.lcc", cs, color_id);

	return ESP_OK;
}


esp_err_t start_rest_server(const char *base_path, int core_id) {
    if (NULL == base_path) {
            ESP_LOGE(LOGTAG_REST, "wrong base path");
            return ESP_FAIL;
    }
    rest_server_context_t *rest_context = (rest_server_context_t *)calloc(1, sizeof(rest_server_context_t));
    if (NULL == rest_context) {
            ESP_LOGE(LOGTAG_REST, "No memory for rest context");
            return ESP_FAIL;
    }
    strlcpy(rest_context->base_path, base_path, sizeof(rest_context->base_path));

    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.core_id = core_id;

    ESP_LOGI(LOGTAG_REST, "Starting HTTP Server");
    if (ESP_OK != httpd_start(&server, &config)) {
            ESP_LOGE(LOGTAG_REST, "Start server failed");
            free(rest_context);
            return ESP_FAIL;
    }

    /* URI handler for getting light controller config */
    httpd_uri_t lc_config_get_uri = {
        .uri = "/api/v1/lc/getconfig",
        .method = HTTP_GET,
        .handler = lc_config_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &lc_config_get_uri);

    /* URI handler for light controller config */
    httpd_uri_t lc_config_post_uri = {
        .uri = "/api/v1/lc/setconfig",
        .method = HTTP_POST,
        .handler = lc_config_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &lc_config_post_uri);

    /* URI handler for updating color definition */
    httpd_uri_t lc_cols_get_uri = {
        .uri = "/api/v1/lc/getcols",
        .method = HTTP_GET,
        .handler = lc_cols_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &lc_cols_get_uri);


    /* URI handler for updating color definition */
    httpd_uri_t lc_coldef_post_uri = {
        .uri = "/api/v1/lc/coldef",
        .method = HTTP_POST,
        .handler = lc_coldef_post_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &lc_coldef_post_uri);

    /* URI handler for getting web server files */
    httpd_uri_t common_get_uri = {
        .uri = "/*",
        .method = HTTP_GET,
        .handler = rest_common_get_handler,
        .user_ctx = rest_context
    };
    httpd_register_uri_handler(server, &common_get_uri);

    return ESP_OK;
}
