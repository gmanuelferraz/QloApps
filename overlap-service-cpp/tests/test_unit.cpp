// overlap-service-cpp/tests/test_unit.cpp
#include "doctest.h"
#include "OverlapDetector.hpp"
#include <vector>

TEST_SUITE("unit") {

TEST_CASE("OverlapDetector::isLeapYear calculates leap years correctly") {
    SUBCASE("Standard non-leap years") {
        CHECK_FALSE(OverlapDetector::isLeapYear(2023));
        CHECK_FALSE(OverlapDetector::isLeapYear(2025));
        CHECK_FALSE(OverlapDetector::isLeapYear(2026));
    }

    SUBCASE("Standard leap years (divisible by 4, not 100)") {
        CHECK(OverlapDetector::isLeapYear(2020));
        CHECK(OverlapDetector::isLeapYear(2024));
        CHECK(OverlapDetector::isLeapYear(2028));
    }

    SUBCASE("Centurial years (divisible by 100)") {
        CHECK_FALSE(OverlapDetector::isLeapYear(1700));
        CHECK_FALSE(OverlapDetector::isLeapYear(1800));
        CHECK_FALSE(OverlapDetector::isLeapYear(1900));
        CHECK_FALSE(OverlapDetector::isLeapYear(2100));

        // Divisible by 400
        CHECK(OverlapDetector::isLeapYear(1600));
        CHECK(OverlapDetector::isLeapYear(2000));
        CHECK(OverlapDetector::isLeapYear(2400));
    }
}

TEST_CASE("OverlapDetector::daysInMonth returns accurate days for all months") {
    SUBCASE("February in common vs leap year") {
        CHECK(OverlapDetector::daysInMonth(2026, 2) == 28);
        CHECK(OverlapDetector::daysInMonth(2024, 2) == 29);
        CHECK(OverlapDetector::daysInMonth(2000, 2) == 29);
        CHECK(OverlapDetector::daysInMonth(1900, 2) == 28);
    }

    SUBCASE("Months with 31 days") {
        CHECK(OverlapDetector::daysInMonth(2026, 1) == 31);
        CHECK(OverlapDetector::daysInMonth(2026, 3) == 31);
        CHECK(OverlapDetector::daysInMonth(2026, 5) == 31);
        CHECK(OverlapDetector::daysInMonth(2026, 7) == 31);
        CHECK(OverlapDetector::daysInMonth(2026, 8) == 31);
        CHECK(OverlapDetector::daysInMonth(2026, 10) == 31);
        CHECK(OverlapDetector::daysInMonth(2026, 12) == 31);
    }

    SUBCASE("Months with 30 days") {
        CHECK(OverlapDetector::daysInMonth(2026, 4) == 30);
        CHECK(OverlapDetector::daysInMonth(2026, 6) == 30);
        CHECK(OverlapDetector::daysInMonth(2026, 9) == 30);
        CHECK(OverlapDetector::daysInMonth(2026, 11) == 30);
    }

    SUBCASE("Invalid month values") {
        CHECK(OverlapDetector::daysInMonth(2026, 0) == 0);
        CHECK(OverlapDetector::daysInMonth(2026, 13) == 0);
        CHECK(OverlapDetector::daysInMonth(2026, -1) == 0);
    }
}

TEST_CASE("OverlapDetector::parseDateToDays validates and converts ISO dates") {
    SUBCASE("Valid dates") {
        auto d1 = OverlapDetector::parseDateToDays("2026-09-01");
        CHECK(d1.has_value());

        auto d2 = OverlapDetector::parseDateToDays("2026-09-05");
        CHECK(d2.has_value());
        CHECK(*d2 - *d1 == 4);

        // Leap day
        CHECK(OverlapDetector::parseDateToDays("2024-02-29").has_value());

        // Epoch date (1970-01-01)
        auto epoch = OverlapDetector::parseDateToDays("1970-01-01");
        REQUIRE(epoch.has_value());
        CHECK(*epoch == 0);
    }

    SUBCASE("Invalid calendar dates") {
        // Feb 29 in non-leap year
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-02-29").has_value());
        // Day out of range for 30-day month
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-09-31").has_value());
        // Month out of range
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-00-01").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-13-01").has_value());
        // Day out of range (zero)
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-09-00").has_value());
    }

    SUBCASE("Malformed formats and strings") {
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-9-1").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-09-1").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("26-09-01").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026/09/01").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-09-01T00:00:00").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-0a-01").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("2026-09-0b").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("").has_value());
        CHECK_FALSE(OverlapDetector::parseDateToDays("not-a-date").has_value());
    }
}

TEST_CASE("OverlapDetector::calculateOverlapNights correctly counts night spans") {
    CHECK(OverlapDetector::calculateOverlapNights("2026-09-01", "2026-09-05") == 4);
    CHECK(OverlapDetector::calculateOverlapNights("2026-09-03", "2026-09-05") == 2);
    CHECK(OverlapDetector::calculateOverlapNights("2026-09-01", "2026-09-02") == 1);

    // Over leap day span
    CHECK(OverlapDetector::calculateOverlapNights("2024-02-28", "2024-03-01") == 2);

    // Non-overlapping or degenerate ranges
    CHECK(OverlapDetector::calculateOverlapNights("2026-09-05", "2026-09-05") == 0);
    CHECK(OverlapDetector::calculateOverlapNights("2026-09-05", "2026-09-01") == 0);
    CHECK(OverlapDetector::calculateOverlapNights("invalid", "2026-09-05") == 0);
    CHECK(OverlapDetector::calculateOverlapNights("2026-09-01", "invalid") == 0);
}

TEST_CASE("OverlapDetector::isValidReservationDates enforces check_in < check_out") {
    CHECK(OverlapDetector::isValidReservationDates("2026-09-01", "2026-09-05") == true);
    CHECK(OverlapDetector::isValidReservationDates("2026-09-01", "2026-09-02") == true);

    // Same-day check-in and check-out is not allowed for night-based lodging
    CHECK(OverlapDetector::isValidReservationDates("2026-09-05", "2026-09-05") == false);

    // Inverted dates
    CHECK(OverlapDetector::isValidReservationDates("2026-09-05", "2026-09-01") == false);

    // Invalid dates
    CHECK(OverlapDetector::isValidReservationDates("invalid", "2026-09-05") == false);
    CHECK(OverlapDetector::isValidReservationDates("2026-09-01", "invalid") == false);
    CHECK(OverlapDetector::isValidReservationDates("2026-02-29", "2026-03-01") == false);
}

TEST_CASE("OverlapDetector::detectOverlaps handles empty and single reservation batches") {
    SUBCASE("Empty batch") {
        std::vector<Reservation> emptyBatch;
        AuditResult res = OverlapDetector::detectOverlaps(emptyBatch);
        CHECK(res.totalReservationsAudited == 0);
        CHECK(res.totalRoomsAudited == 0);
        CHECK(res.totalConflictsFound == 0);
        CHECK(res.conflicts.empty());
    }

    SUBCASE("Single reservation batch") {
        std::vector<Reservation> single = {
            {"RES-001", "101", "2026-09-01", "2026-09-05", "Guest Single"}
        };
        AuditResult res = OverlapDetector::detectOverlaps(single);
        CHECK(res.totalReservationsAudited == 1);
        CHECK(res.totalRoomsAudited == 1);
        CHECK(res.totalConflictsFound == 0);
        CHECK(res.conflicts.empty());
    }
}

TEST_CASE("OverlapDetector::detectOverlaps RN-001: Consecutive reservations do not conflict") {
    // Semi-open hotel interval [check_in, check_out): checkout on 2026-09-05 frees the room for checkin on 2026-09-05
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Guest A"},
        {"RES-002", "101", "2026-09-05", "2026-09-10", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    CHECK(res.totalReservationsAudited == 2);
    CHECK(res.totalRoomsAudited == 1);
    CHECK(res.totalConflictsFound == 0);
    CHECK(res.conflicts.empty());
}

TEST_CASE("OverlapDetector::detectOverlaps RN-002: Partial overlap detection") {
    // 2 nights overlap: 2026-09-03 to 2026-09-05 -> MEDIUM severity (< 3 nights)
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Guest A"},
        {"RES-002", "101", "2026-09-03", "2026-09-07", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    REQUIRE(res.totalConflictsFound == 1);
    REQUIRE(res.conflicts.size() == 1);

    const auto& c = res.conflicts[0];
    CHECK(c.roomId == "101");
    CHECK(c.reservationAId == "RES-001");
    CHECK(c.reservationBId == "RES-002");
    CHECK(c.overlapStart == "2026-09-03");
    CHECK(c.overlapEnd == "2026-09-05");
    CHECK(c.overlapNights == 2);
    CHECK(c.severity == "MEDIUM");
    CHECK(c.message.find("2 noites") != std::string::npos);
}

TEST_CASE("OverlapDetector::detectOverlaps RN-003: Encapsulated overlap detection") {
    // 3 nights overlap: 2026-09-03 to 2026-09-06 -> HIGH severity (>= 3 nights)
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-10", "Guest A"},
        {"RES-002", "101", "2026-09-03", "2026-09-06", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    REQUIRE(res.totalConflictsFound == 1);
    REQUIRE(res.conflicts.size() == 1);

    const auto& c = res.conflicts[0];
    CHECK(c.roomId == "101");
    CHECK(c.reservationAId == "RES-001");
    CHECK(c.reservationBId == "RES-002");
    CHECK(c.overlapStart == "2026-09-03");
    CHECK(c.overlapEnd == "2026-09-06");
    CHECK(c.overlapNights == 3);
    CHECK(c.severity == "HIGH");
    CHECK(c.message.find("3 noites") != std::string::npos);
}

TEST_CASE("OverlapDetector::detectOverlaps 1-night overlap has MEDIUM severity and singular message") {
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Guest A"},
        {"RES-002", "101", "2026-09-04", "2026-09-07", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    REQUIRE(res.totalConflictsFound == 1);
    const auto& c = res.conflicts[0];
    CHECK(c.overlapNights == 1);
    CHECK(c.severity == "MEDIUM");
    CHECK(c.message.find("1 noite.") != std::string::npos);
}

TEST_CASE("OverlapDetector::detectOverlaps RN-004: Distinct rooms do not conflict") {
    std::vector<Reservation> batch = {
        {"RES-001", "room-101", "2026-09-01", "2026-09-05", "Guest A"},
        {"RES-002", "room-102", "2026-09-01", "2026-09-05", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    CHECK(res.totalReservationsAudited == 2);
    CHECK(res.totalRoomsAudited == 2);
    CHECK(res.totalConflictsFound == 0);
    CHECK(res.conflicts.empty());
}

TEST_CASE("OverlapDetector::detectOverlaps Requirements Section 9 Fixture Batch") {
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Carlos Eduardo"},
        {"RES-002", "101", "2026-09-03", "2026-09-07", "Mariana Lima"},
        {"RES-003", "101", "2026-09-07", "2026-09-10", "Fernanda Rocha"},
        {"RES-004", "102", "2026-09-01", "2026-09-08", "Pedro Alcantara"},
        {"RES-005", "102", "2026-09-04", "2026-09-07", "Julia Martins"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    CHECK(res.totalReservationsAudited == 5);
    CHECK(res.totalRoomsAudited == 2);
    REQUIRE(res.totalConflictsFound == 2);
    REQUIRE(res.conflicts.size() == 2);

    // Conflict 1: RES-001 & RES-002 in room 101 (2 nights, MEDIUM)
    CHECK(res.conflicts[0].roomId == "101");
    CHECK(res.conflicts[0].reservationAId == "RES-001");
    CHECK(res.conflicts[0].reservationBId == "RES-002");
    CHECK(res.conflicts[0].overlapStart == "2026-09-03");
    CHECK(res.conflicts[0].overlapEnd == "2026-09-05");
    CHECK(res.conflicts[0].overlapNights == 2);
    CHECK(res.conflicts[0].severity == "MEDIUM");

    // Conflict 2: RES-004 & RES-005 in room 102 (3 nights, HIGH)
    CHECK(res.conflicts[1].roomId == "102");
    CHECK(res.conflicts[1].reservationAId == "RES-004");
    CHECK(res.conflicts[1].reservationBId == "RES-005");
    CHECK(res.conflicts[1].overlapStart == "2026-09-04");
    CHECK(res.conflicts[1].overlapEnd == "2026-09-07");
    CHECK(res.conflicts[1].overlapNights == 3);
    CHECK(res.conflicts[1].severity == "HIGH");
}

TEST_CASE("OverlapDetector::detectOverlaps sorts unsorted input chronologically") {
    // Pass reservations in reverse order
    std::vector<Reservation> batch = {
        {"RES-002", "101", "2026-09-03", "2026-09-07", "Guest B"},
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Guest A"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    REQUIRE(res.totalConflictsFound == 1);
    CHECK(res.conflicts[0].reservationAId == "RES-001");
    CHECK(res.conflicts[0].reservationBId == "RES-002");
    CHECK(res.conflicts[0].overlapNights == 2);
}

} // TEST_SUITE("unit")
