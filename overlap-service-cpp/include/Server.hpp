#ifndef SERVER_HPP
#define SERVER_HPP

#include "httplib.h"

/**
 * Configures the HTTP endpoints on the given httplib::Server instance.
 * Endpoints:
 *  - GET /healthz
 *  - POST /v1/inventory-audits/overlaps
 */
void setupServerRoutes(httplib::Server& svr);

#endif // SERVER_HPP
