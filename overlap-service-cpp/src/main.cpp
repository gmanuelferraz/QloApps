// overlap-service-cpp/src/main.cpp
#include <iostream>
#include "Server.hpp"

int main() {
    httplib::Server svr;
    setupServerRoutes(svr);

    std::cout << "[QLO-FEAT-007] Servico de Auditoria C++ escutando em http://127.0.0.1:8107" << std::endl;
    svr.listen("127.0.0.1", 8107);
    return 0;
}
