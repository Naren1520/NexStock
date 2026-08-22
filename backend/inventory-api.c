#define _GNU_SOURCE
#include <microhttpd.h>
#include <cjson/cJSON.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_PORT 8040
#define MAX_BODY 65536
#define INVENTORY_FILE "backend/inventory.json"
#define FRONTEND_ROOT "frontend"

typedef struct {
    char *body;
    size_t length;
} RequestBody;

static const char *method_or_empty(const char *value) {
    return value ? value : "";
}

static cJSON *load_inventory(void) {
    FILE *file = fopen(INVENTORY_FILE, "rb");
    long size;
    char *text;
    cJSON *root;

    if (!file) {
        root = cJSON_CreateObject();
        cJSON_AddArrayToObject(root, "products");
        cJSON_AddArrayToObject(root, "rentals");
        return root;
    }

    fseek(file, 0, SEEK_END);
    size = ftell(file);
    rewind(file);
    text = calloc((size_t)size + 1, 1);
    if (!text || fread(text, 1, (size_t)size, file) != (size_t)size) {
        free(text);
        fclose(file);
        return NULL;
    }
    fclose(file);

    root = cJSON_Parse(text);
    free(text);
    if (!root || !cJSON_IsObject(root)) {
        cJSON_Delete(root);
        return NULL;
    }
    if (!cJSON_IsArray(cJSON_GetObjectItem(root, "products"))) {
        cJSON_AddArrayToObject(root, "products");
    }
    if (!cJSON_IsArray(cJSON_GetObjectItem(root, "rentals"))) {
        cJSON_AddArrayToObject(root, "rentals");
    }
    return root;
}

static int save_inventory(const cJSON *root) {
    FILE *file;
    char *text = cJSON_Print(root);
    if (!text) return 0;

    file = fopen(INVENTORY_FILE, "wb");
    if (!file) {
        free(text);
        return 0;
    }
    fputs(text, file);
    fclose(file);
    free(text);
    return 1;
}

static cJSON *json_response(int success, const char *message) {
    cJSON *response = cJSON_CreateObject();
    cJSON_AddBoolToObject(response, "success", success);
    if (message) cJSON_AddStringToObject(response, success ? "message" : "error", message);
    return response;
}

static cJSON *find_product(cJSON *products, int id) {
    cJSON *product;
    cJSON_ArrayForEach(product, products) {
        cJSON *value = cJSON_GetObjectItem(product, "id");
        if (cJSON_IsNumber(value) && value->valueint == id) return product;
    }
    return NULL;
}

static int product_index(cJSON *products, int id) {
    int index = 0;
    cJSON *product;
    cJSON_ArrayForEach(product, products) {
        cJSON *value = cJSON_GetObjectItem(product, "id");
        if (cJSON_IsNumber(value) && value->valueint == id) {
            return index;
        }
        index++;
    }
    return -1;
}

static int body_int(cJSON *body, const char *name, int fallback) {
    cJSON *value = cJSON_GetObjectItem(body, name);
    return cJSON_IsNumber(value) ? value->valueint : fallback;
}

static double body_double(cJSON *body, const char *name, double fallback) {
    cJSON *value = cJSON_GetObjectItem(body, name);
    return cJSON_IsNumber(value) ? value->valuedouble : fallback;
}

static const char *body_string(cJSON *body, const char *name) {
    cJSON *value = cJSON_GetObjectItem(body, name);
    return cJSON_IsString(value) ? value->valuestring : "";
}

static cJSON *handle_api(const char *method, const char *url, const char *body_text, int *status) {
    cJSON *body = body_text && body_text[0] ? cJSON_Parse(body_text) : cJSON_CreateObject();
    cJSON *root = NULL;
    cJSON *products;
    cJSON *result = NULL;
    int id;

    *status = 200;
    if (!body || !cJSON_IsObject(body)) {
        cJSON_Delete(body);
        *status = 400;
        return json_response(0, "Invalid JSON");
    }

    if (!strcmp(url, "/api/chatbot-key") && !strcmp(method, "GET")) {
        const char *key = getenv("GEMINI_API_KEY");
        cJSON_Delete(body);
        if (!key || !key[0]) {
            *status = 500;
            return json_response(0, "GEMINI_API_KEY not configured");
        }
        result = cJSON_CreateObject();
        cJSON_AddStringToObject(result, "apiKey", key);
        return result;
    }

    root = load_inventory();
    cJSON_Delete(body);
    if (!root) {
        *status = 500;
        return json_response(0, "Unable to read inventory.json");
    }
    products = cJSON_GetObjectItem(root, "products");

    if (!strcmp(url, "/api/c/products") && !strcmp(method, "GET")) {
        result = cJSON_Duplicate(products, 1);
    } else if (!strcmp(url, "/api/c/rentals") && !strcmp(method, "GET")) {
        result = cJSON_Duplicate(cJSON_GetObjectItem(root, "rentals"), 1);
    } else {
        cJSON *request = body_text && body_text[0] ? cJSON_Parse(body_text) : cJSON_CreateObject();
        if (!request) request = cJSON_CreateObject();
        id = body_int(request, "id", body_int(request, "productId", -1));

        if (!strcmp(url, "/api/c/product/add") && !strcmp(method, "POST")) {
            if (find_product(products, id)) {
                *status = 409;
                result = json_response(0, "Product ID already exists");
            } else {
                cJSON *product = cJSON_CreateObject();
                cJSON_AddNumberToObject(product, "id", id);
                cJSON_AddStringToObject(product, "name", body_string(request, "name"));
                cJSON_AddNumberToObject(product, "price", body_double(request, "price", 0));
                cJSON_AddNumberToObject(product, "quantity", body_int(request, "quantity", 0));
                cJSON_AddItemToArray(products, product);
                result = json_response(save_inventory(root), "Product added");
                if (!cJSON_IsTrue(cJSON_GetObjectItem(result, "success"))) *status = 500;
                else *status = 201;
            }
        } else if (!strcmp(url, "/api/c/product/delete") && !strcmp(method, "DELETE")) {
            cJSON *product = find_product(products, id);
            if (!product) {
                *status = 404;
                result = json_response(0, "Product not found");
            } else {
                cJSON_DeleteItemFromArray(products, product_index(products, id));
                result = json_response(save_inventory(root), "Product deleted");
                if (!cJSON_IsTrue(cJSON_GetObjectItem(result, "success"))) *status = 500;
            }
        } else if (!strcmp(url, "/api/c/product/update") && !strcmp(method, "PUT")) {
            cJSON *product = find_product(products, id);
            if (!product) {
                *status = 404;
                result = json_response(0, "Product not found");
            } else {
                cJSON_ReplaceItemInObject(product, "name", cJSON_CreateString(body_string(request, "name")));
                cJSON_ReplaceItemInObject(product, "price", cJSON_CreateNumber(body_double(request, "price", 0)));
                cJSON_ReplaceItemInObject(product, "quantity", cJSON_CreateNumber(body_int(request, "quantity", 0)));
                result = json_response(save_inventory(root), "Product updated");
                if (!cJSON_IsTrue(cJSON_GetObjectItem(result, "success"))) *status = 500;
            }
        } else if (!strcmp(url, "/api/c/product/sell") && !strcmp(method, "POST")) {
            cJSON *product = find_product(products, id);
            int quantity = body_int(request, "quantitySold", 0);
            int available = product ? body_int(product, "quantity", 0) : 0;
            if (!product) {
                *status = 400;
                result = json_response(0, "Product not found");
            } else if (available < quantity) {
                *status = 400;
                result = json_response(0, "Insufficient quantity");
                cJSON_AddNumberToObject(result, "available", available);
            } else {
                cJSON_ReplaceItemInObject(product, "quantity", cJSON_CreateNumber(available - quantity));
                result = json_response(save_inventory(root), "Sale processed");
                cJSON_AddNumberToObject(result, "quantity_remaining", available - quantity);
                if (!cJSON_IsTrue(cJSON_GetObjectItem(result, "success"))) *status = 500;
            }
        } else if (!strcmp(url, "/api/c/product/search/") || !strncmp(url, "/api/c/product/search/", 21)) {
            int search_id = atoi(url + 21);
            cJSON *product = find_product(products, search_id);
            if (!product) {
                *status = 404;
                result = json_response(0, "Product not found");
            } else result = cJSON_Duplicate(product, 1);
        } else if (!strncmp(url, "/api/c/product/sort/", 21) && !strcmp(method, "GET")) {
            int count = cJSON_GetArraySize(products);
            for (int i = 0; i < count - 1; i++) {
                for (int j = i + 1; j < count; j++) {
                    cJSON *left = cJSON_GetArrayItem(products, i);
                    cJSON *right = cJSON_GetArrayItem(products, j);
                    int swap = 0;
                    if (!strcmp(url + 21, "id")) swap = body_int(left, "id", 0) > body_int(right, "id", 0);
                    else if (!strcmp(url + 21, "price")) swap = body_double(left, "price", 0) > body_double(right, "price", 0);
                    else swap = strcasecmp(body_string(left, "name"), body_string(right, "name")) > 0;
                    if (swap) {
                        cJSON_DetachItemFromArray(products, j);
                        cJSON_InsertItemInArray(products, i, right);
                    }
                }
            }
            result = json_response(save_inventory(root), "Products sorted");
            cJSON_AddItemToObject(result, "products", cJSON_Duplicate(products, 1));
        } else if (!strcmp(url, "/api/c/rental/record") && !strcmp(method, "POST")) {
            cJSON *product = find_product(products, body_int(request, "productId", -1));
            cJSON *rental;
            int quantity;
            if (!product) {
                *status = 400;
                result = json_response(0, "Product not found");
            } else if ((quantity = body_int(product, "quantity", 0)) <= 0) {
                *status = 400;
                result = json_response(0, "Product not available for rent");
            } else {
                rental = cJSON_CreateObject();
                cJSON_AddNumberToObject(rental, "rentalId", (double)time(NULL) * 1000 + rand() % 1000);
                cJSON_AddNumberToObject(rental, "productId", body_int(request, "productId", 0));
                cJSON_AddStringToObject(rental, "productName", body_string(product, "name"));
                cJSON_AddStringToObject(rental, "renterName", body_string(request, "renterName"));
                cJSON_AddStringToObject(rental, "rentDate", "");
                cJSON_AddStringToObject(rental, "returnDate", body_string(request, "returnDate"));
                cJSON_AddStringToObject(rental, "phoneNumber", body_string(request, "phoneNumber"));
                cJSON_AddStringToObject(rental, "address", body_string(request, "address"));
                cJSON_AddNumberToObject(rental, "amountPaid", body_double(request, "amountPaid", 0));
                cJSON_AddStringToObject(rental, "status", "active");
                cJSON_AddItemToArray(cJSON_GetObjectItem(root, "rentals"), rental);
                cJSON_ReplaceItemInObject(product, "quantity", cJSON_CreateNumber(quantity - 1));
                result = json_response(save_inventory(root), "Rental recorded");
                cJSON_AddItemToObject(result, "rental", cJSON_Duplicate(rental, 1));
                if (!cJSON_IsTrue(cJSON_GetObjectItem(result, "success"))) *status = 500;
                else *status = 201;
            }
        } else if (!strcmp(url, "/api/c/rental/return") && !strcmp(method, "PUT")) {
            cJSON *rentals = cJSON_GetObjectItem(root, "rentals");
            cJSON *rental = NULL;
            int count = cJSON_GetArraySize(rentals);
            for (int i = 0; i < count; i++) {
                cJSON *candidate = cJSON_GetArrayItem(rentals, i);
                if (body_int(request, "rentalId", -1) == (int)cJSON_GetNumberValue(cJSON_GetObjectItem(candidate, "rentalId"))) {
                    rental = candidate;
                    break;
                }
            }
            if (!rental) {
                *status = 404;
                result = json_response(0, "Rental not found");
            } else {
                cJSON_ReplaceItemInObject(rental, "status", cJSON_CreateString("returned"));
                cJSON *product = find_product(products, body_int(rental, "productId", -1));
                if (product) cJSON_ReplaceItemInObject(product, "quantity", cJSON_CreateNumber(body_int(product, "quantity", 0) + 1));
                result = json_response(save_inventory(root), "Rental marked as returned");
                if (!cJSON_IsTrue(cJSON_GetObjectItem(result, "success"))) *status = 500;
            }
        } else {
            *status = 404;
            result = json_response(0, "API endpoint not found");
        }
        cJSON_Delete(request);
    }

    cJSON_Delete(root);
    return result ? result : json_response(0, "Request failed");
}

static enum MHD_Result send_json(struct MHD_Connection *connection, cJSON *json, unsigned int status) {
    char *text = cJSON_PrintUnformatted(json);
    struct MHD_Response *response = MHD_create_response_from_buffer(strlen(text), text, MHD_RESPMEM_MUST_FREE);
    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
    return MHD_queue_response(connection, status, response);
}

static enum MHD_Result send_file(struct MHD_Connection *connection, const char *url) {
    char path[1024];
    const char *relative = !strcmp(url, "/") ? "/index.html" : url;
    const char *content_type = "text/plain";
    struct stat file_info;
    int file;

    if (strstr(relative, "..") || strchr(relative, '\\')) {
        return MHD_queue_response(connection, 403,
                                  MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT));
    }
    snprintf(path, sizeof(path), "%s%s", FRONTEND_ROOT, relative);
    file = open(path, O_RDONLY);
    if (file < 0 || fstat(file, &file_info) != 0 || !S_ISREG(file_info.st_mode)) {
        if (file >= 0) close(file);
        return MHD_queue_response(connection, 404,
                                  MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT));
    }
    if (strstr(path, ".html")) content_type = "text/html";
    else if (strstr(path, ".css")) content_type = "text/css";
    else if (strstr(path, ".js")) content_type = "application/javascript";
    else if (strstr(path, ".json")) content_type = "application/json";
    else if (strstr(path, ".png")) content_type = "image/png";
    else if (strstr(path, ".jpg") || strstr(path, ".jpeg")) content_type = "image/jpeg";

    struct MHD_Response *response = MHD_create_response_from_fd((uint64_t)file_info.st_size, file);
    MHD_add_response_header(response, "Content-Type", content_type);
    return MHD_queue_response(connection, 200, response);
}

static enum MHD_Result handler(void *cls, struct MHD_Connection *connection, const char *url,
                               const char *method, const char *version, const char *upload_data,
                               size_t *upload_data_size, void **con_cls) {
    RequestBody *request;
    int status;
    cJSON *result;
    (void)cls; (void)version;

    if (!strcmp(method_or_empty(method), "OPTIONS")) {
        struct MHD_Response *response = MHD_create_response_from_buffer(0, "", MHD_RESPMEM_PERSISTENT);
        MHD_add_response_header(response, "Access-Control-Allow-Origin", "*");
        MHD_add_response_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type");
        return MHD_queue_response(connection, 204, response);
    }

    if (!*con_cls) {
        request = calloc(1, sizeof(RequestBody));
        *con_cls = request;
        return MHD_YES;
    }
    request = *con_cls;
    if (*upload_data_size) {
        if (request->length + *upload_data_size > MAX_BODY) return MHD_NO;
        request->body = realloc(request->body, request->length + *upload_data_size + 1);
        memcpy(request->body + request->length, upload_data, *upload_data_size);
        request->length += *upload_data_size;
        request->body[request->length] = '\0';
        *upload_data_size = 0;
        return MHD_YES;
    }
    if (strncmp(url, "/api/", 5) == 0) {
        result = handle_api(method_or_empty(method), url, request->body, &status);
        free(request->body);
        free(request);
        *con_cls = NULL;
        enum MHD_Result response = send_json(connection, result, status);
        cJSON_Delete(result);
        return response;
    }
    free(request->body);
    free(request);
    *con_cls = NULL;
    return send_file(connection, url);
}

static void request_completed(void *cls, struct MHD_Connection *connection, void **con_cls,
                              enum MHD_RequestTerminationCode code) {
    RequestBody *request = con_cls ? *con_cls : NULL;
    (void)cls; (void)connection; (void)code;
    if (request) {
        free(request->body);
        free(request);
        *con_cls = NULL;
    }
}

int main(void) {
    const char *port_text = getenv("PORT");
    unsigned short port = (unsigned short)(port_text ? atoi(port_text) : DEFAULT_PORT);
    struct MHD_Daemon *daemon;
    srand((unsigned int)time(NULL));
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, port, NULL, NULL, &handler, NULL,
                              MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL, MHD_OPTION_END);
    if (!daemon) {
        fprintf(stderr, "Failed to start C backend on port %u\n", port);
        return 1;
    }
    printf("NexStock C backend listening on port %u\n", port);
    while (1) sleep(3600);
    MHD_stop_daemon(daemon);
    return 0;
}
