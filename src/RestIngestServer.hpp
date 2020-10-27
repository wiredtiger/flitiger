
#include <cpprest/uri.h>
#include <cpprest/http_listener.h>

class RestIngestServer
{
public:
    RestIngestServer(utility::string_t address = U("http://127.0.0.1:8099"));
    int start();

private:
    utility::string_t _address;
};
