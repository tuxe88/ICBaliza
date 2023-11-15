#include "http_server.h"

char status_wifi_ssid[32];
char status_wifi_password[32];
int status_max_retries;
static char status_repo_name[128];
static char status_repo_owner[64];
static char status_repo_authorization[64];

const char* htmlForm = "<html>\n"
                      "<head>\n"
                      "<title>ICBaliza configuration</title>\n"
                      "<link rel=\"stylesheet\" href=\"https://fonts.googleapis.com/css2?family=Roboto:wght@400&display=swap\">\n"
                      "<style>\n"
                      "  body { display: flex; align-items: center; justify-content: center; height: 100vh; background-color: #f4f4f4; font-family: 'Roboto', sans-serif; }\n"
                      "  form { background-color: #fff; padding: 20px; border-radius: 8px; box-shadow: 0 0 10px rgba(0, 0, 0, 0.1); max-width: 400px; width: 100%; text-align: center; }\n"
                      "  label { display: block; margin-bottom: 8px; }\n"
                      "  input { width: 100%; padding: 8px; margin-bottom: 16px; box-sizing: border-box; border: 1px solid #ccc; border-radius: 4px; }\n"
                      "  input[type=\"submit\"] { background-color: #4caf50; color: #fff; cursor: pointer; }\n"
                      "  h1 { color: #333; font-size: 28px; margin-bottom: 20px; }\n"
                      "</style>\n"
                      "</head>\n"
                      "<body>\n"
                      "<form action=\"submit\" method=\"post\">\n"

                      "  <h1>ICBaliza</h1>\n"

                      "  <label for=\"ssid\">SSID:</label>\n"
                      "  <input type=\"text\" id=\"ssid\" name=\"ssid\"><br><br>\n"

                      "  <label for=\"password\">Password:</label>\n"
                      "  <input type=\"password\" id=\"password\" name=\"password\"><br><br>\n"

                      "  <label for=\"retry\">Retry Number:</label>\n"
                      "  <input type=\"number\" id=\"retry\" name=\"retry\"><br><br>\n"

                      "  <label for=\"repository\">Repository name:</label>\n"
                      "  <input type=\"text\" id=\"repository\" name=\"repository\"><br><br>\n"

                      "  <label for=\"owner\">Repository Owner:</label>\n"
                      "  <input type=\"text\" id=\"owner\" name=\"owner\"><br><br>\n"

                      "  <label for=\"authorization\">Authorization:</label>\n"
                      "  <input type=\"text\" id=\"authorization\" name=\"authorization\"><br><br>\n"

                      "  <input type=\"submit\" value=\"Submit\">\n"
                      "</form>\n"
                      "</body>\n"
                      "</html>\n";


const char* htmlMessage = "<!DOCTYPE html>\n"
                          "<html lang=\"en\">\n"
                          "<head>\n"
                          "    <meta charset=\"UTF-8\">\n"
                          "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
                          "    <title>ICBaliza Configuration successful</title>\n"
                          "    <style>\n"
                          "        body {\n"
                          "            display: flex;\n"
                          "            align-items: center;\n"
                          "            justify-content: center;\n"
                          "            height: 100vh;\n"
                          "            background-color: #f4f4f4;\n"
                          "            font-family: Arial, sans-serif;\n"
                          "        }\n"
                          "\n"
                          "        table {\n"
                          "            border-collapse: collapse;\n"
                          "            width: 50%;\n"
                          "            text-align: center;\n"
                          "        }\n"
                          "\n"
                          "        th, td {\n"
                          "            padding: 20px;\n"
                          "            border: 1px solid #ddd;\n"
                          "        }\n"
                          "\n"
                          "        h1 {\n"
                          "            color: #333;\n"
                          "            font-size: 24px;\n"
                          "            margin-bottom: 20px;\n"
                          "        }\n"
                          "    </style>\n"
                          "</head>\n"
                          "<body>\n"
                          "    <table>\n"
                          "        <tr>\n"
                          "            <th colspan=\"2\">\n"
                          "                <h1>ICBaliza Configuration</h1>\n"
                          "            </th>\n"
                          "        </tr>\n"
                          "        <tr>\n"
                          "            <td colspan=\"2\">\n"
                          "                <p>ICBaliza was configured successfully with the received parameters. If the blue light persists after 45 seconds, please check the parameters sent and try again.</p>\n"
                          "            </td>\n"
                          "        </tr>\n"
                          "    </table>\n"
                          "</body>\n"
                          "</html>\n";    


esp_err_t root_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Accept", "text/html");
    httpd_resp_send(req, htmlForm, strlen(htmlForm));
    return ESP_OK;
}

esp_err_t root_handler_post(httpd_req_t *req)
{
    char chunk[512];  
    size_t received = 0;

    while (1) {
        ssize_t ret = httpd_req_recv(req, chunk, sizeof(chunk));

        if (ret <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                httpd_resp_send_408(req);
            }
            break;
        }

        received += ret;

        if (received >= req->content_len) {
            break;
        }
    }

    chunk[received] = '\0';

    char ssid[32];
    char password[32];
    char retry[32];
    char repository[128];
    char owner[64];
    char authorization[64];

    if (httpd_query_key_value(chunk, "ssid", ssid, sizeof(ssid)) == ESP_OK &&
        httpd_query_key_value(chunk, "password", password, sizeof(password)) == ESP_OK &&
        httpd_query_key_value(chunk, "retry", retry, sizeof(retry)) == ESP_OK &&
        httpd_query_key_value(chunk, "repository", repository, sizeof(repository)) == ESP_OK &&
        httpd_query_key_value(chunk, "owner", owner, sizeof(owner)) == ESP_OK &&
        httpd_query_key_value(chunk, "authorization", authorization, sizeof(authorization)) == ESP_OK) {

        printf("SSID: %s\n", ssid);
        printf("Password: **********\n");
        printf("Retry: %s\n", retry);
        printf("Repository: %s\n", repository);
        printf("Owner: %s\n", owner);
        printf("Authorization: %s\n", authorization);

        esp_wifi_disconnect();

        wifi_config_t wifi_configuration_sta = {
            .sta = {
                .ssid = "",
                .password = "",
            },
        };

        strcpy((char *)wifi_configuration_sta.sta.ssid, ssid); 
        strcpy((char *)wifi_configuration_sta.sta.password, password);

        strcpy(status_wifi_ssid, ssid);
        strcpy(status_wifi_password, password); 
        strcpy(status_repo_name, repository); 
        strcpy(status_repo_owner, owner);
        strcpy(status_repo_authorization, authorization);
        status_max_retries = 5;

        esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration_sta);

        esp_wifi_connect();

        httpd_resp_send(req, htmlMessage, strlen(htmlMessage));

        return ESP_OK;
    } else {

        const char* responseError = "An error ocurred!";
        httpd_resp_send(req, responseError, strlen(responseError));
        return ESP_FAIL;
    }
}

static httpd_uri_t root = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = root_handler,
    .user_ctx = NULL,
};

static httpd_uri_t rootPost = {
    .uri = "/submit",
    .method = HTTP_POST,
    .handler = root_handler_post,
    .user_ctx = NULL,
};

void init_http_server() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 8; 
    config.max_resp_headers = 8 * 1024; 
    config.server_port = PORT;
    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &root);
        httpd_register_uri_handler(server, &rootPost);
    }
}

char* get_repo_name(){
    return status_repo_name;
}

char* get_repo_owner(){
    return status_repo_owner;
}

char* get_repo_authorization(){
    return status_repo_authorization;
}