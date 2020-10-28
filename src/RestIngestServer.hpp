
#include <cpprest/uri.h>
#include <cpprest/http_listener.h>
#include <cpprest/json.h>

class RestIngestServer
{
public:
    RestIngestServer(void (*json_callback)(web::json::value), utility::string_t address = U("http://127.0.0.1:8099")):_address(address), _json_callback(json_callback) {};
    int start();
private:
    utility::string_t _address;
    void (*_json_callback)(web::json::value);
};
