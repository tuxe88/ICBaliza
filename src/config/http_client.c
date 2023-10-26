#include "http_client.h"
#include <esp_http_server.h>
#include "esp_http_client.h"

static void get_workflows_github(){
    esp_http_client_config_t config_get = {
        .url = "https://api.github.com/repos/"REPO_OWNER"/"REPO_NAME"/actions/runs?per_page=1&page=1",
        .method = HTTP_METHOD_GET,
        .transport_type = HTTP_TRANSPORT_OVER_SSL,  //Specify transport type
        .crt_bundle_attach = esp_crt_bundle_attach, //Attach the certificate bundle 
        .event_handler = client_event_get_handler  
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    //char *post_data = "{\"saa\":\"dsdssdadsa\"}";
    //esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client,"Content-Type","application/vnd.github+json");
    esp_http_client_set_header(client,"Authorization","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");
    esp_http_client_set_header(client,"X-GitHub-Api-Version","2022-11-28");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);    
}
