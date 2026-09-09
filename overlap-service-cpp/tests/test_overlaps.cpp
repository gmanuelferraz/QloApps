#include "OverlapDetector.hpp"
#include <iostream>
#include <cassert>
#include <vector>

void testDateValidation() {
    std::cout << "[TEST] Date validation & leap years..." << std::endl;

    // Leap year checks
    assert(OverlapDetector::isLeapYear(2024) == true);
    assert(OverlapDetector::isLeapYear(2026) == false);
    assert(OverlapDetector::isLeapYear(2000) == true);
    assert(OverlapDetector::isLeapYear(1900) == false);

    // Days in month
    assert(OverlapDetector::daysInMonth(2024, 2) == 29);
    assert(OverlapDetector::daysInMonth(2026, 2) == 28);
    assert(OverlapDetector::daysInMonth(2026, 4) == 30);
    assert(OverlapDetector::daysInMonth(2026, 12) == 31);
    assert(OverlapDetector::daysInMonth(2026, 13) == 0);

    // Strict YYYY-MM-DD parsing
    assert(OverlapDetector::parseDateToDays("2026-09-01").has_value());
    assert(OverlapDetector::parseDateToDays("2024-02-29").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-02-29").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-9-1").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-09-1").has_value());
    assert(!OverlapDetector::parseDateToDays("26-09-01").has_value());
    assert(!OverlapDetector::parseDateToDays("2026/09/01").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-09-01T00:00:00").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-13-01").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-00-01").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-09-00").has_value());
    assert(!OverlapDetector::parseDateToDays("2026-09-31").has_value());
    assert(!OverlapDetector::parseDateToDays("").has_value());

    // Reservation dates validity (checkIn < checkOut)
    assert(OverlapDetector::isValidReservationDates("2026-09-01", "2026-09-05") == true);
    assert(OverlapDetector::isValidReservationDates("2026-09-05", "2026-09-05") == false);
    assert(OverlapDetector::isValidReservationDates("2026-09-05", "2026-09-01") == false);
    assert(OverlapDetector::isValidReservationDates("invalid", "2026-09-05") == false);

    std::cout << "  -> PASSED" << std::endl;
}

void testRN001ConsecutiveNoConflict() {
    std::cout << "[TEST] RN-001: Consecutive reservations on checkout day do not overlap..." << std::endl;
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Guest A"},
        {"RES-002", "101", "2026-09-05", "2026-09-10", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    assert(res.totalReservationsAudited == 2);
    assert(res.totalRoomsAudited == 1);
    assert(res.totalConflictsFound == 0);
    assert(res.conflicts.empty());

    std::cout << "  -> PASSED" << std::endl;
}

void testRN002PartialOverlap() {
    std::cout << "[TEST] RN-002: Partial overlap detection (2 nights, MEDIUM)..." << std::endl;
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Guest A"},
        {"RES-002", "101", "2026-09-03", "2026-09-07", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    assert(res.totalConflictsFound == 1);
    const auto& c = res.conflicts[0];
    assert(c.roomId == "101");
    assert(c.reservationAId == "RES-001");
    assert(c.reservationBId == "RES-002");
    assert(c.overlapStart == "2026-09-03");
    assert(c.overlapEnd == "2026-09-05");
    assert(c.overlapNights == 2);
    assert(c.severity == "MEDIUM");

    std::cout << "  -> PASSED" << std::endl;
}

void testRN003EncapsulatedOverlap() {
    std::cout << "[TEST] RN-003: Encapsulated overlap (3 nights, HIGH)..." << std::endl;
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-10", "Guest A"},
        {"RES-002", "101", "2026-09-03", "2026-09-06", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    assert(res.totalConflictsFound == 1);
    const auto& c = res.conflicts[0];
    assert(c.roomId == "101");
    assert(c.reservationAId == "RES-001");
    assert(c.reservationBId == "RES-002");
    assert(c.overlapStart == "2026-09-03");
    assert(c.overlapEnd == "2026-09-06");
    assert(c.overlapNights == 3);
    assert(c.severity == "HIGH");

    std::cout << "  -> PASSED" << std::endl;
}

void testRN004RoomIsolation() {
    std::cout << "[TEST] RN-004: Distinct rooms do not conflict despite same dates..." << std::endl;
    std::vector<Reservation> batch = {
        {"RES-001", "room-101", "2026-09-01", "2026-09-05", "Guest A"},
        {"RES-002", "room-102", "2026-09-01", "2026-09-05", "Guest B"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    assert(res.totalRoomsAudited == 2);
    assert(res.totalConflictsFound == 0);

    std::cout << "  -> PASSED" << std::endl;
}

void testRequirementsFixtureDemo() {
    std::cout << "[TEST] Requirements Section 9 Fixture Batch..." << std::endl;
    std::vector<Reservation> batch = {
        {"RES-001", "101", "2026-09-01", "2026-09-05", "Carlos Eduardo"},
        {"RES-002", "101", "2026-09-03", "2026-09-07", "Mariana Lima"},
        {"RES-003", "101", "2026-09-07", "2026-09-10", "Fernanda Rocha"},
        {"RES-004", "102", "2026-09-01", "2026-09-08", "Pedro Alcantara"},
        {"RES-005", "102", "2026-09-04", "2026-09-07", "Julia Martins"}
    };

    AuditResult res = OverlapDetector::detectOverlaps(batch);
    assert(res.totalReservationsAudited == 5);
    assert(res.totalRoomsAudited == 2);
    assert(res.totalConflictsFound == 2);

    // Conflict 1: RES-001 & RES-002 in room 101 (2 nights, MEDIUM)
    assert(res.conflicts[0].roomId == "101");
    assert(res.conflicts[0].overlapNights == 2);
    assert(res.conflicts[0].severity == "MEDIUM");

    // Conflict 2: RES-004 & RES-005 in room 102 (3 nights, HIGH)
    assert(res.conflicts[1].roomId == "102");
    assert(res.conflicts[1].overlapNights == 3);
    assert(res.conflicts[1].severity == "HIGH");

    std::cout << "  -> PASSED" << std::endl;
}

int main() {
    std::cout << "=========================================" << std::endl;
    std::cout << " Running OverlapDetector Unit Tests      " << std::endl;
    std::cout << "=========================================" << std::endl;

    testDateValidation();
    testRN001ConsecutiveNoConflict();
    testRN002PartialOverlap();
    testRN003EncapsulatedOverlap();
    testRN004RoomIsolation();
    testRequirementsFixtureDemo();

    std::cout << "=========================================" << std::endl;
    std::cout << " All unit tests PASSED successfully!     " << std::endl;
    std::cout << "=========================================" << std::endl;

    return 0;
}
