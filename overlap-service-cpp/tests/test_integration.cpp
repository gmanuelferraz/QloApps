// overlap-service-cpp/tests/test_integration.cpp
#include "doctest.h"
#include "Server.hpp"
#include "json.hpp"
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace {

/**
 * RAII fixture to spin up the HTTP server on an OS-assigned ephemeral port
 * and safely terminate it on destruction.
 */
class ServerFixture {
public:
    ServerFixture() {
        setupServerRoutes(server_);
        port_ = server_.bind_to_any_port("127.0.0.1");
        thread_ = std::thread([this]() {
            server_.listen_after_bind();
        });
        server_.wait_until_ready();
    }

    ~ServerFixture() {
        server_.stop();
        if (thread_.joinable()) {
            thread_.join();
        }
    }

    httplib::Client getClient() const {
        httplib::Client cli("127.0.0.1", port_);
        cli.set_connection_timeout(std::chrono::seconds(3));
        cli.set_read_timeout(std::chrono::seconds(3));
        return cli;
    }

    int port() const {
        return port_;
    }

private:
    httplib::Server server_;
    int port_ = 0;
    std::thread thread_;
};

} // anonymous namespace

TEST_SUITE("integration") {

TEST_CASE("HTTP GET /healthz returns 200 and UP status") {
    ServerFixture fixture;
    auto client = fixture.getClient();

    auto res = client.Get("/healthz");
    REQUIRE(res != nullptr);
    CHECK(res->status == 200);
    CHECK(res->get_header_value("Content-Type") == "application/json");

    auto body = json::parse(res->body);
    CHECK(body["status"] == "UP");
}

TEST_CASE("HTTP POST /v1/inventory-audits/overlaps - Successful audits") {
    ServerFixture fixture;
    auto client = fixture.getClient();

    SUBCASE("RN-001: Consecutive bookings without overlap return 0 conflicts") {
        json payload = {
            {"audit_batch_id", "batch-consecutive"},
            {"reservations", json::array({
                {
                    {"reservation_id", "RES-001"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-01"},
                    {"check_out", "2026-09-05"},
                    {"guest_name", "Guest A"}
                },
                {
                    {"reservation_id", "RES-002"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-05"},
                    {"check_out", "2026-09-10"},
                    {"guest_name", "Guest B"}
                }
            })}
        };

        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 200);
        CHECK(res->get_header_value("Content-Type") == "application/json");

        auto body = json::parse(res->body);
        CHECK(body["audit_batch_id"] == "batch-consecutive");
        CHECK(body["total_reservations_audited"] == 2);
        CHECK(body["total_rooms_audited"] == 1);
        CHECK(body["total_conflicts_found"] == 0);
        CHECK(body["conflicts"].is_array());
        CHECK(body["conflicts"].empty());
    }

    SUBCASE("RN-002: Partial overlap returns conflict with MEDIUM severity") {
        json payload = {
            {"reservations", json::array({
                {
                    {"reservation_id", "RES-001"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-01"},
                    {"check_out", "2026-09-05"},
                    {"guest_name", "Guest A"}
                },
                {
                    {"reservation_id", "RES-002"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-03"},
                    {"check_out", "2026-09-07"},
                    {"guest_name", "Guest B"}
                }
            })}
        };

        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 200);

        auto body = json::parse(res->body);
        CHECK(body["total_conflicts_found"] == 1);
        REQUIRE(body["conflicts"].size() == 1);

        const auto& c = body["conflicts"][0];
        CHECK(c["room_id"] == "101");
        CHECK(c["reservation_a_id"] == "RES-001");
        CHECK(c["reservation_b_id"] == "RES-002");
        CHECK(c["overlap_start"] == "2026-09-03");
        CHECK(c["overlap_end"] == "2026-09-05");
        CHECK(c["overlap_nights"] == 2);
        CHECK(c["severity"] == "MEDIUM");
    }

    SUBCASE("RN-003: Encapsulated overlap returns conflict with HIGH severity") {
        json payload = {
            {"reservations", json::array({
                {
                    {"reservation_id", "RES-001"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-01"},
                    {"check_out", "2026-09-10"},
                    {"guest_name", "Guest A"}
                },
                {
                    {"reservation_id", "RES-002"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-03"},
                    {"check_out", "2026-09-06"},
                    {"guest_name", "Guest B"}
                }
            })}
        };

        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 200);

        auto body = json::parse(res->body);
        CHECK(body["total_conflicts_found"] == 1);
        REQUIRE(body["conflicts"].size() == 1);

        const auto& c = body["conflicts"][0];
        CHECK(c["overlap_nights"] == 3);
        CHECK(c["severity"] == "HIGH");
    }

    SUBCASE("Section 9 Fixture Batch returns exact aggregated statistics and conflicts") {
        json payload = {
            {"audit_batch_id", "batch-sec-9"},
            {"reservations", json::array({
                {{"reservation_id", "RES-001"}, {"room_id", "101"}, {"check_in", "2026-09-01"}, {"check_out", "2026-09-05"}, {"guest_name", "Carlos Eduardo"}},
                {{"reservation_id", "RES-002"}, {"room_id", "101"}, {"check_in", "2026-09-03"}, {"check_out", "2026-09-07"}, {"guest_name", "Mariana Lima"}},
                {{"reservation_id", "RES-003"}, {"room_id", "101"}, {"check_in", "2026-09-07"}, {"check_out", "2026-09-10"}, {"guest_name", "Fernanda Rocha"}},
                {{"reservation_id", "RES-004"}, {"room_id", "102"}, {"check_in", "2026-09-01"}, {"check_out", "2026-09-08"}, {"guest_name", "Pedro Alcantara"}},
                {{"reservation_id", "RES-005"}, {"room_id", "102"}, {"check_in", "2026-09-04"}, {"check_out", "2026-09-07"}, {"guest_name", "Julia Martins"}}
            })}
        };

        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 200);

        auto body = json::parse(res->body);
        CHECK(body["audit_batch_id"] == "batch-sec-9");
        CHECK(body["total_reservations_audited"] == 5);
        CHECK(body["total_rooms_audited"] == 2);
        CHECK(body["total_conflicts_found"] == 2);
        REQUIRE(body["conflicts"].size() == 2);

        // Conflict 1: Room 101, MEDIUM (2 nights)
        CHECK(body["conflicts"][0]["room_id"] == "101");
        CHECK(body["conflicts"][0]["overlap_nights"] == 2);
        CHECK(body["conflicts"][0]["severity"] == "MEDIUM");

        // Conflict 2: Room 102, HIGH (3 nights)
        CHECK(body["conflicts"][1]["room_id"] == "102");
        CHECK(body["conflicts"][1]["overlap_nights"] == 3);
        CHECK(body["conflicts"][1]["severity"] == "HIGH");
    }

    SUBCASE("Correlation ID header is propagated; defaults to corr-demo when missing") {
        httplib::Headers headers = {
            {"X-Correlation-ID", "corr-test-custom-12345"}
        };
        json payload = {
            {"reservations", json::array({
                {{"reservation_id", "RES-001"}, {"room_id", "101"}, {"check_in", "2026-09-01"}, {"check_out", "2026-09-05"}, {"guest_name", "Guest A"}}
            })}
        };

        // With X-Correlation-ID header
        auto res1 = client.Post("/v1/inventory-audits/overlaps", headers, payload.dump(), "application/json");
        REQUIRE(res1 != nullptr);
        CHECK(res1->status == 200);
        auto body1 = json::parse(res1->body);
        CHECK(body1["correlation_id"] == "corr-test-custom-12345");

        // Without header -> default
        auto res2 = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res2 != nullptr);
        CHECK(res2->status == 200);
        auto body2 = json::parse(res2->body);
        CHECK(body2["correlation_id"] == "corr-demo");
        CHECK(body2["audit_batch_id"] == "batch-default");
    }
}

TEST_CASE("HTTP POST /v1/inventory-audits/overlaps - RFC 7807 Error Handling (400 Bad Request)") {
    ServerFixture fixture;
    auto client = fixture.getClient();

    SUBCASE("Malformed JSON body") {
        auto res = client.Post("/v1/inventory-audits/overlaps", "{ bad json content", "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
        CHECK(res->get_header_value("Content-Type") == "application/problem+json");

        auto body = json::parse(res->body);
        CHECK(body["status"] == 400);
        CHECK(body["type"] == "https://hotel.local/errors/invalid-audit-batch");
        CHECK(body["title"] == "Lote de Auditoria Inválido");
        CHECK(body["instance"] == "/v1/inventory-audits/overlaps");
    }

    SUBCASE("Missing reservations key") {
        json payload = {{"audit_batch_id", "batch-123"}};
        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
        CHECK(res->get_header_value("Content-Type") == "application/problem+json");
    }

    SUBCASE("Reservations is not an array") {
        json payload = {{"reservations", "not an array"}};
        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
    }

    SUBCASE("Batch exceeds maximum allowed limit of 200 reservations") {
        json reservations = json::array();
        for (int i = 0; i < 201; ++i) {
            reservations.push_back({
                {"reservation_id", "RES-" + std::to_string(i)},
                {"room_id", "101"},
                {"check_in", "2026-09-01"},
                {"check_out", "2026-09-05"},
                {"guest_name", "Guest"}
            });
        }
        json payload = {{"reservations", reservations}};

        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
        auto body = json::parse(res->body);
        CHECK(body["detail"] == "O lote de reservas excede o limite máximo permitido de 200 registros.");
    }

    SUBCASE("Missing required field in reservation item") {
        json payload = {
            {"reservations", json::array({
                {
                    {"reservation_id", "RES-001"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-01"},
                    // missing check_out
                    {"guest_name", "Guest"}
                }
            })}
        };
        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
        auto body = json::parse(res->body);
        CHECK(body["detail"] == "Campos obrigatórios ausentes ou com tipos inválidos em uma ou mais reservas.");
    }

    SUBCASE("Non-string type for reservation field") {
        json payload = {
            {"reservations", json::array({
                {
                    {"reservation_id", "RES-001"},
                    {"room_id", 101}, // integer instead of string
                    {"check_in", "2026-09-01"},
                    {"check_out", "2026-09-05"},
                    {"guest_name", "Guest"}
                }
            })}
        };
        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
    }

    SUBCASE("Invalid date format or calendar value") {
        json payload = {
            {"reservations", json::array({
                {
                    {"reservation_id", "RES-001"},
                    {"room_id", "101"},
                    {"check_in", "2026/09/01"}, // invalid delimiter
                    {"check_out", "2026-09-05"},
                    {"guest_name", "Guest"}
                }
            })}
        };
        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
        auto body = json::parse(res->body);
        CHECK(body["detail"] == "Data de check-in ou check-out com formato ou valor inválido (esperado: YYYY-MM-DD).");
    }

    SUBCASE("Check-out date not strictly after check-in date") {
        json payload = {
            {"reservations", json::array({
                {
                    {"reservation_id", "RES-001"},
                    {"room_id", "101"},
                    {"check_in", "2026-09-05"},
                    {"check_out", "2026-09-05"}, // same date
                    {"guest_name", "Guest"}
                }
            })}
        };
        auto res = client.Post("/v1/inventory-audits/overlaps", payload.dump(), "application/json");
        REQUIRE(res != nullptr);
        CHECK(res->status == 400);
        auto body = json::parse(res->body);
        CHECK(body["detail"] == "A data de check-out deve ser estritamente posterior à data de check-in.");
    }
}

TEST_CASE("HTTP Unmatched endpoints or methods return 404") {
    ServerFixture fixture;
    auto client = fixture.getClient();

    auto res1 = client.Get("/v1/inventory-audits/overlaps");
    REQUIRE(res1 != nullptr);
    CHECK(res1->status == 404);

    auto res2 = client.Get("/non-existent-path");
    REQUIRE(res2 != nullptr);
    CHECK(res2->status == 404);
}

} // TEST_SUITE("integration")
