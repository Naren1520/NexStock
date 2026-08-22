#define _GNU_SOURCE
#include <microhttpd.h>
#include <cjson/cJSON.h>
#include <bson/bson.h>
#include <mongoc/mongoc.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#define DEFAULT_PORT 8040
#define MAX_BODY 65536
#define FRONTEND_ROOT "frontend"
#define MAX_SESSIONS 256

typedef struct {
    char *body;
    size_t length;
} RequestBody;

typedef struct {
    char token[65];
    char email[256];
} Session;

static mongoc_client_t *mongo_client;
static mongoc_database_t *mongo_database;
static Session sessions[MAX_SESSIONS];
static pthread_mutex_t sessions_mutex = PTHREAD_MUTEX_INITIALIZER;

static cJSON *empty_inventory(void) {
    cJSON *root = cJSON_CreateObject();
    cJSON_AddArrayToObject(root, "products");
    cJSON_AddArrayToObject(root, "rentals");
    return root;
}

static cJSON *load_collection(const char *name) {
    mongoc_collection_t *collection = mongoc_database_get_collection(mongo_database, name);
    mongoc_cursor_t *cursor = mongoc_collection_find_with_opts(collection, NULL, NULL, NULL);
    const bson_t *document;
    cJSON *array = cJSON_CreateArray();

    while (mongoc_cursor_next(cursor, &document)) {
        size_t json_length = 0;
        char *json = bson_as_relaxed_extended_json(document, &json_length);
        cJSON *item = cJSON_ParseWithLength(json, json_length);
        if (item && cJSON_IsObject(item)) {
            cJSON_DeleteItemFromObject(item, "_id");
            cJSON_AddItemToArray(array, item);
        } else {
            cJSON_Delete(item);
        }
        bson_free(json);
    }
    if (mongoc_cursor_error(cursor, NULL)) {
        cJSON_Delete(array);
        array = NULL;
    }
    mongoc_cursor_destroy(cursor);
    mongoc_collection_destroy(collection);
    return array;
}

static cJSON *load_inventory(void) {
    cJSON *root = empty_inventory();
    cJSON *products = load_collection("products");
    cJSON *rentals = load_collection("rentals");
    if (!products || !rentals) {
        cJSON_Delete(products);
        cJSON_Delete(rentals);
        cJSON_Delete(root);
        return NULL;
    }
    cJSON_ReplaceItemInObject(root, "products", products);
    cJSON_ReplaceItemInObject(root, "rentals", rentals);
    return root;
}

static int save_collection(const char *name, const cJSON *array) {
    mongoc_collection_t *collection = mongoc_database_get_collection(mongo_database, name);
    bson_t query = BSON_INITIALIZER;
    bson_t options = BSON_INITIALIZER;
    cJSON *item;
    int success = mongoc_collection_delete_many(collection, &query, &options, NULL, NULL);
    cJSON_ArrayForEach(item, array) {
        char *json = cJSON_PrintUnformatted(item);
        bson_error_t error;
        bson_t *document = bson_new_from_json((const uint8_t *)json, -1, &error);
        if (!document || !mongoc_collection_insert_one(collection, document, NULL, NULL, &error)) success = 0;
        bson_destroy(document);
        free(json);
    }
    bson_destroy(&query);
    bson_destroy(&options);
    mongoc_collection_destroy(collection);
    return success;
}

static int save_inventory(const cJSON *root) {
    return save_collection("products", cJSON_GetObjectItem(root, "products")) &&
           save_collection("rentals", cJSON_GetObjectItem(root, "rentals"));
}

static void sha256_hex(const char *value, char output[65]) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char *)value, strlen(value), digest);
    for (size_t i = 0; i < sizeof(digest); i++) sprintf(output + (i * 2), "%02x", digest[i]);
    output[64] = '\0';
}

static void token_hex(char output[65]) {
    unsigned char bytes[32];
    if (RAND_bytes(bytes, sizeof(bytes)) != 1) {
        for (size_t i = 0; i < sizeof(bytes); i++) bytes[i] = (unsigned char)(rand() & 0xff);
    }
    for (size_t i = 0; i < sizeof(bytes); i++) sprintf(output + (i * 2), "%02x", bytes[i]);
    output[64] = '\0';
}

static int session_valid(const char *token) {
    int valid = 0;
    if (!token || !token[0]) return 0;
    pthread_mutex_lock(&sessions_mutex);
    for (size_t i = 0; i < MAX_SESSIONS; i++) {
        if (!strcmp(sessions[i].token, token)) { valid = 1; break; }
    }
    pthread_mutex_unlock(&sessions_mutex);
    return valid;
}

static void add_session(const char *token, const char *email) {
    pthread_mutex_lock(&sessions_mutex);
    size_t slot = 0;
    for (size_t i = 0; i < MAX_SESSIONS; i++) if (!sessions[i].token[0]) { slot = i; break; }
    snprintf(sessions[slot].token, sizeof(sessions[slot].token), "%s", token);
    snprintf(sessions[slot].email, sizeof(sessions[slot].email), "%s", email);
    pthread_mutex_unlock(&sessions_mutex);
}

static const char *bearer_token(struct MHD_Connection *connection) {
    const char *header = MHD_lookup_connection_value(connection, MHD_HEADER_KIND, "Authorization");
    return header && !strncasecmp(header, "Bearer ", 7) ? header + 7 : NULL;
}

static const char *method_or_empty(const char *value) {
    return value ? value : "";
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

    if ((!strcmp(url, "/api/auth/signup") || !strcmp(url, "/api/auth/login")) &&
        !strcmp(method, "POST")) {
        const char *email = body_string(body, "email");
        const char *password = body_string(body, "password");
        char password_hash[65];
        bson_t query = BSON_INITIALIZER;
        bson_t user_document = BSON_INITIALIZER;
        bson_error_t error;
        mongoc_collection_t *users = mongoc_database_get_collection(mongo_database, "users");
        const bson_t *found = NULL;
        mongoc_cursor_t *cursor;

        if (!email[0] || !password[0] || strlen(email) >= 256 || strlen(password) < 8) {
            cJSON_Delete(body);
            *status = 400;
            mongoc_collection_destroy(users);
            return json_response(0, "Email and a password of at least 8 characters are required");
        }
        BSON_APPEND_UTF8(&query, "email", email);
        cursor = mongoc_collection_find_with_opts(users, &query, NULL, NULL);
        mongoc_cursor_next(cursor, &found);
        if (!strcmp(url, "/api/auth/signup") && found) {
            mongoc_cursor_destroy(cursor);
            bson_destroy(&query);
            mongoc_collection_destroy(users);
            cJSON_Delete(body);
            *status = 409;
            return json_response(0, "Email already registered");
        }
        if (!strcmp(url, "/api/auth/login") && !found) {
            mongoc_cursor_destroy(cursor);
            bson_destroy(&query);
            mongoc_collection_destroy(users);
            cJSON_Delete(body);
            *status = 401;
            return json_response(0, "Invalid email or password");
        }
        sha256_hex(password, password_hash);
        if (!strcmp(url, "/api/auth/login")) {
            bson_iter_t iterator;
            const char *stored_hash = NULL;
            if (bson_iter_init_find(&iterator, found, "passwordHash") && BSON_ITER_HOLDS_UTF8(&iterator))
                stored_hash = bson_iter_utf8(&iterator, NULL);
            if (!stored_hash || strcmp(stored_hash, password_hash)) {
                mongoc_cursor_destroy(cursor);
                bson_destroy(&query);
                mongoc_collection_destroy(users);
                cJSON_Delete(body);
                *status = 401;
                return json_response(0, "Invalid email or password");
            }
        } else {
            BSON_APPEND_UTF8(&user_document, "email", email);
            BSON_APPEND_UTF8(&user_document, "passwordHash", password_hash);
            BSON_APPEND_DATE_TIME(&user_document, "createdAt", (int64_t)time(NULL) * 1000);
            if (!mongoc_collection_insert_one(users, &user_document, NULL, NULL, &error)) {
                mongoc_cursor_destroy(cursor);
                bson_destroy(&query);
                bson_destroy(&user_document);
                mongoc_collection_destroy(users);
                cJSON_Delete(body);
                *status = 500;
                return json_response(0, error.message);
            }
        }
        char token[65];
        token_hex(token);
        add_session(token, email);
        result = cJSON_CreateObject();
        cJSON_AddBoolToObject(result, "success", 1);
        cJSON_AddStringToObject(result, "token", token);
        cJSON *user = cJSON_CreateObject();
        cJSON_AddStringToObject(user, "email", email);
        cJSON_AddItemToObject(result, "user", user);
        mongoc_cursor_destroy(cursor);
        bson_destroy(&query);
        bson_destroy(&user_document);
        mongoc_collection_destroy(users);
        cJSON_Delete(body);
        *status = !strcmp(url, "/api/auth/signup") ? 201 : 200;
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
        MHD_add_response_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
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
        if (!strncmp(url, "/api/c/", 7) && !session_valid(bearer_token(connection))) {
            free(request->body);
            free(request);
            *con_cls = NULL;
            result = json_response(0, "Authentication required");
            enum MHD_Result response = send_json(connection, result, MHD_HTTP_UNAUTHORIZED);
            cJSON_Delete(result);
            return response;
        }
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
    const char *mongo_uri = getenv("MONGODB_URI");
    const char *database_name = getenv("MONGODB_DATABASE");
    unsigned short port = (unsigned short)(port_text ? atoi(port_text) : DEFAULT_PORT);
    mongoc_uri_t *uri;
    bson_error_t error;
    bson_t ping = BSON_INITIALIZER;
    struct MHD_Daemon *daemon;
    srand((unsigned int)time(NULL));
    if (!mongo_uri || !mongo_uri[0]) {
        fprintf(stderr, "MONGODB_URI is required\n");
        return 1;
    }
    mongoc_init();
    uri = mongoc_uri_new_with_error(mongo_uri, &error);
    if (!uri) {
        fprintf(stderr, "Invalid MONGODB_URI: %s\n", error.message);
        mongoc_cleanup();
        return 1;
    }
    mongo_client = mongoc_client_new_from_uri(uri);
    mongoc_uri_destroy(uri);
    if (!mongo_client) {
        fprintf(stderr, "Unable to create MongoDB client\n");
        mongoc_cleanup();
        return 1;
    }
    mongo_database = mongoc_client_get_database(mongo_client, database_name && database_name[0] ? database_name : "nexstock");
    BSON_APPEND_INT32(&ping, "ping", 1);
    if (!mongoc_database_command_simple(mongo_database, &ping, NULL, NULL, &error)) {
        fprintf(stderr, "Unable to connect to MongoDB: %s\n", error.message);
        bson_destroy(&ping);
        mongoc_database_destroy(mongo_database);
        mongoc_client_destroy(mongo_client);
        mongoc_cleanup();
        return 1;
    }
    bson_destroy(&ping);
    daemon = MHD_start_daemon(MHD_USE_INTERNAL_POLLING_THREAD, port, NULL, NULL, &handler, NULL,
                              MHD_OPTION_NOTIFY_COMPLETED, &request_completed, NULL, MHD_OPTION_END);
    if (!daemon) {
        fprintf(stderr, "Failed to start C backend on port %u\n", port);
        mongoc_database_destroy(mongo_database);
        mongoc_client_destroy(mongo_client);
        mongoc_cleanup();
        return 1;
    }
    printf("NexStock C backend listening on port %u\n", port);
    while (1) sleep(3600);
    MHD_stop_daemon(daemon);
    mongoc_database_destroy(mongo_database);
    mongoc_client_destroy(mongo_client);
    mongoc_cleanup();
    return 0;
}
