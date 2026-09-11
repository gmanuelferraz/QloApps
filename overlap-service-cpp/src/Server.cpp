// overlap-service-cpp/src/Server.cpp
#include "Server.hpp"
#include "json.hpp"
#include "OverlapDetector.hpp"
#include <vector>
#include <string>

using json = nlohmann::json;

void setupServerRoutes(httplib::Server& svr) {
    svr.Get("/healthz", [](const httplib::Request&, httplib::Response& res) {
        res.set_content("{\"status\":\"UP\"}", "application/json");
    });

    svr.Post("/v1/inventory-audits/overlaps", [](const httplib::Request& req, httplib::Response& res) {
        auto sendProblemResponse = [&res, &req](int status, const std::string& type, const std::string& title, const std::string& detail) {
            json error;
            error["type"] = type;
            error["title"] = title;
            error["status"] = status;
            error["detail"] = detail;
            error["instance"] = req.path;

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

            std::vector<Reservation> reservations;
            reservations.reserve(body["reservations"].size());

            for (const auto& item : body["reservations"]) {
                if (!item.is_object() ||
                    !item.contains("reservation_id") || !item.contains("room_id") ||
                    !item.contains("check_in") || !item.contains("check_out") ||
                    !item.contains("guest_name") ||
                    !item["reservation_id"].is_string() || !item["room_id"].is_string() ||
                    !item["check_in"].is_string() || !item["check_out"].is_string() ||
                    !item["guest_name"].is_string()) {
                    sendProblemResponse(400, "https://hotel.local/errors/invalid-audit-batch", "Lote de Auditoria Inválido", "Campos obrigatórios ausentes ou com tipos inválidos em uma ou mais reservas.");
                    return;
                }

                std::string checkIn = item["check_in"].get<std::string>();
                std::string checkOut = item["check_out"].get<std::string>();

                auto startDays = OverlapDetector::parseDateToDays(checkIn);
                auto endDays = OverlapDetector::parseDateToDays(checkOut);

                if (!startDays.has_value() || !endDays.has_value()) {
                    sendProblemResponse(400, "https://hotel.local/errors/invalid-audit-batch", "Lote de Auditoria Inválido", "Data de check-in ou check-out com formato ou valor inválido (esperado: YYYY-MM-DD).");
                    return;
                }

                if (!OverlapDetector::isValidReservationDates(checkIn, checkOut)) {
                    sendProblemResponse(400, "https://hotel.local/errors/invalid-audit-batch", "Lote de Auditoria Inválido", "A data de check-out deve ser estritamente posterior à data de check-in.");
                    return;
                }

                reservations.push_back({
                    item["reservation_id"].get<std::string>(),
                    item["room_id"].get<std::string>(),
                    checkIn,
                    checkOut,
                    item["guest_name"].get<std::string>()
                });
            }

            AuditResult auditResult = OverlapDetector::detectOverlaps(reservations);

            json conflicts = json::array();
            for (const auto& c : auditResult.conflicts) {
                json item;
                item["room_id"] = c.roomId;
                item["reservation_a_id"] = c.reservationAId;
                item["reservation_b_id"] = c.reservationBId;
                item["overlap_start"] = c.overlapStart;
                item["overlap_end"] = c.overlapEnd;
                item["overlap_nights"] = c.overlapNights;
                item["severity"] = c.severity;
                item["message"] = c.message;
                conflicts.push_back(item);
            }

            json response;
            response["correlation_id"] = req.has_header("X-Correlation-ID") ? req.get_header_value("X-Correlation-ID") : "corr-demo";
            response["audit_batch_id"] = body.value("audit_batch_id", "batch-default");
            response["total_reservations_audited"] = auditResult.totalReservationsAudited;
            response["total_rooms_audited"] = auditResult.totalRoomsAudited;
            response["total_conflicts_found"] = auditResult.totalConflictsFound;
            response["conflicts"] = conflicts;

            res.status = 200;
            res.set_content(response.dump(), "application/json");
        } catch (const std::exception& e) {
            sendProblemResponse(400, "https://hotel.local/errors/invalid-audit-batch", "Lote de Auditoria Inválido", "JSON de reservas inválido ou malformado.");
        }
    });
}
