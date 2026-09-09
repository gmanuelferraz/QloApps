#include "OverlapDetector.hpp"
#include <map>
#include <algorithm>
#include <cctype>
#include <cstdio>

bool OverlapDetector::isLeapYear(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int OverlapDetector::daysInMonth(int year, int month) {
    static const int days[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (month == 2 && isLeapYear(year)) {
        return 29;
    }
    if (month >= 1 && month <= 12) {
        return days[month];
    }
    return 0;
}

std::optional<int> OverlapDetector::parseDateToDays(const std::string& dateStr) {
    if (dateStr.size() != 10) {
        return std::nullopt;
    }
    for (size_t i = 0; i < 10; ++i) {
        if (i == 4 || i == 7) {
            if (dateStr[i] != '-') {
                return std::nullopt;
            }
        } else {
            if (!std::isdigit(static_cast<unsigned char>(dateStr[i]))) {
                return std::nullopt;
            }
        }
    }

    int y = 0, m = 0, d = 0;
    if (std::sscanf(dateStr.c_str(), "%4d-%2d-%2d", &y, &m, &d) != 3) {
        return std::nullopt;
    }

    if (y < 1 || m < 1 || m > 12 || d < 1 || d > daysInMonth(y, m)) {
        return std::nullopt;
    }

    // Howard Hinnant's civil calendar algorithm
    y -= m <= 2;
    const int era = (y >= 0 ? y : y - 399) / 400;
    const unsigned yoe = static_cast<unsigned>(y - era * 400);
    const unsigned doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + static_cast<int>(doe) - 719468;
}

int OverlapDetector::calculateOverlapNights(const std::string& start, const std::string& end) {
    auto daysStart = parseDateToDays(start);
    auto daysEnd = parseDateToDays(end);
    if (!daysStart.has_value() || !daysEnd.has_value()) {
        return 0;
    }
    int diff = *daysEnd - *daysStart;
    return diff > 0 ? diff : 0;
}

bool OverlapDetector::isValidReservationDates(const std::string& checkIn, const std::string& checkOut) {
    auto startDays = parseDateToDays(checkIn);
    auto endDays = parseDateToDays(checkOut);
    if (!startDays.has_value() || !endDays.has_value()) {
        return false;
    }
    return *startDays < *endDays;
}

AuditResult OverlapDetector::detectOverlaps(const std::vector<Reservation>& reservations) {
    AuditResult result;
    result.totalReservationsAudited = static_cast<int>(reservations.size());

    std::map<std::string, std::vector<Reservation>> roomBuckets;
    for (const auto& res : reservations) {
        roomBuckets[res.roomId].push_back(res);
    }
    result.totalRoomsAudited = static_cast<int>(roomBuckets.size());

    for (auto& pair : roomBuckets) {
        auto& list = pair.second;

        std::sort(list.begin(), list.end(), [](const Reservation& a, const Reservation& b) {
            if (a.checkIn != b.checkIn) {
                return a.checkIn < b.checkIn;
            }
            return a.checkOut < b.checkOut;
        });

        for (size_t i = 0; i < list.size(); ++i) {
            const auto& r1 = list[i];
            for (size_t j = i + 1; j < list.size(); ++j) {
                const auto& r2 = list[j];

                // Since list is sorted by checkIn, if r2.checkIn >= r1.checkOut,
                // no subsequent reservation in this room bucket can overlap with r1.
                if (r2.checkIn >= r1.checkOut) {
                    break;
                }

                Conflict c;
                c.roomId = pair.first;
                c.reservationAId = r1.id;
                c.reservationBId = r2.id;
                c.overlapStart = r2.checkIn;
                c.overlapEnd = (r1.checkOut < r2.checkOut) ? r1.checkOut : r2.checkOut;
                c.overlapNights = calculateOverlapNights(c.overlapStart, c.overlapEnd);
                c.severity = (c.overlapNights >= 3) ? "HIGH" : "MEDIUM";
                c.message = "Quarto " + c.roomId + " alocado simultaneamente para " +
                            c.reservationAId + " e " + c.reservationBId + " por " +
                            std::to_string(c.overlapNights) +
                            (c.overlapNights == 1 ? " noite." : " noites.");

                result.conflicts.push_back(std::move(c));
            }
        }
    }

    result.totalConflictsFound = static_cast<int>(result.conflicts.size());
    return result;
}
