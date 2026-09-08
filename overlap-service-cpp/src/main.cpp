// overlap-service-cpp/src/main.cpp
#include <iostream>
#include <vector>
#include <unordered_map>
#include <algorithm>
#include <string>
#include <cstdio>
#include "../include/httplib.h"
#include "../include/json.hpp"

using json = nlohmann::json;

struct Reservation {
    std::string id;
    std::string roomId;
    std::string checkIn;
    std::string checkOut;
    std::string guestName;
};

// Converts ISO date "YYYY-MM-DD" to days since civil epoch (1970-01-01)
static int parseDateToDays(const std::string& dateStr) {
    int y = 0, m = 0, d = 0;
    if (std::sscanf(dateStr.c_str(), "%4d-%2d-%2d", &y, &m, &d) == 3) {
        y -= m <= 2;
        const int era = (y >= 0 ? y : y - 399) / 400;
        const unsigned yoe = static_cast<unsigned>(y - era * 400);
        const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
        const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
        return era * 146097 + static_cast<int>(doe) - 719468;
    }
    return 0;
}

static int calculateOverlapNights(const std::string& start, const std::string& end) {
    int daysStart = parseDateToDays(start);
    int daysEnd = parseDateToDays(end);
    int diff = daysEnd - daysStart;
    return diff > 0 ? diff : 0;
}

int main() {
    httplib::Server svr;

    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"UP\"}", "application/json");
    });

    svr.Post("/v1/inventory-audits/overlaps", [](const httplib::Request& req, httplib::Response& res) {
        auto sendProblemResponse = [&res](int status, const std::string& type, const std::string& title, const std::string& detail) {
            json error;
            error["type"] = type;
            error["title"] = title;
            error["status"] = status;
            error["detail"] = detail;
            error["instance"] = "/v1/inventory-audits/overlaps";

            res.status = status;
            res.set_content(error.dump(), "application/problem+json");
        };

        try {
            auto body = json::parse(req.body);

            if (!body.contains("reservations") || !body["reservations"].is_array()) {
                sendProblemResponse(400, "https://hotel.local/errors/invalid-audit-batch", "Lote de Auditoria Inválido", "JSON de reservas inválido ou malformado.");
                return;
            }

            if (body["reservations"].size() > 200) {
                sendProblemResponse(400, "https://hotel.local/errors/invalid-audit-batch", "Lote de Auditoria Inválido", "O lote de reservas excede o limite máximo permitido de 200 registros.");
                return;
            }

            std::unordered_map<std::string, std::vector<Reservation>> roomBuckets;
            for (const auto& item : body["reservations"]) {
                roomBuckets[item["room_id"]].push_back({
                    item["reservation_id"], item["room_id"], item["check_in"], item["check_out"], item["guest_name"]
                });
            }

            json conflicts = json::array();
            int totalAudited = 0;

            for (auto& pair : roomBuckets) {
                auto& list = pair.second;
                totalAudited += list.size();
                
                std::sort(list.begin(), list.end(), [](const Reservation& a, const Reservation& b) {
                    return a.checkIn < b.checkIn;
                });

                for (size_t i = 0; i + 1 < list.size(); ++i) {
                    const auto& r1 = list[i];
                    const auto& r2 = list[i + 1];

                    if (r2.checkIn < r1.checkOut) {
                        json c;
                        std::string overlapStart = r2.checkIn;
                        std::string overlapEnd = (r1.checkOut < r2.checkOut) ? r1.checkOut : r2.checkOut;
                        int overlapNights = calculateOverlapNights(overlapStart, overlapEnd);

                        c["room_id"] = pair.first;
                        c["reservation_a_id"] = r1.id;
                        c["reservation_b_id"] = r2.id;
                        c["overlap_start"] = overlapStart;
                        c["overlap_end"] = overlapEnd;
                        c["overlap_nights"] = overlapNights;
                        c["severity"] = (overlapNights >= 3) ? "HIGH" : "MEDIUM";
                        c["message"] = "Colisao de ocupacao detectada no quarto " + pair.first;
                        conflicts.push_back(c);
                    }
                }
            }

            json response;
            response["correlation_id"] = req.has_header("X-Correlation-ID") ? req.get_header_value("X-Correlation-ID") : "corr-demo";
            response["audit_batch_id"] = body.value("audit_batch_id", "batch-default");
            response["total_reservations_audited"] = totalAudited;
            response["total_rooms_audited"] = roomBuckets.size();
            response["total_conflicts_found"] = conflicts.size();
            response["conflicts"] = conflicts;

            res.status = 200;
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            sendProblemResponse(400, "https://hotel.local/errors/invalid-audit-batch", "Lote de Auditoria Inválido", "JSON de reservas inválido ou malformado.");
        }
    });

    std::cout << "[QLO-FEAT-007] Servico de Auditoria C++ escutando em http://127.0.0.1:8107" << std::endl;
    svr.listen("127.0.0.1", 8107);
    return 0;
}
