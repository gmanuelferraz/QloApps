#ifndef OVERLAP_DETECTOR_HPP
#define OVERLAP_DETECTOR_HPP

#include <string>
#include <vector>
#include <optional>

/**
 * Represents a hotel room reservation.
 */
struct Reservation {
    std::string id;
    std::string roomId;
    std::string checkIn;
    std::string checkOut;
    std::string guestName;
};

/**
 * Represents an overlap conflict between two reservations in the same physical room.
 */
struct Conflict {
    std::string roomId;
    std::string reservationAId;
    std::string reservationBId;
    std::string overlapStart;
    std::string overlapEnd;
    int overlapNights = 0;
    std::string severity; // "HIGH" (>= 3 nights) or "MEDIUM" (1 to 2 nights)
    std::string message;
};

/**
 * Represents the aggregated audit report.
 */
struct AuditResult {
    int totalReservationsAudited = 0;
    int totalRoomsAudited = 0;
    int totalConflictsFound = 0;
    std::vector<Conflict> conflicts;
};

/**
 * Core engine for validating dates and detecting inventory allocation overlaps.
 */
class OverlapDetector {
public:
    /**
     * Checks if a given year is a leap year in the Gregorian calendar.
     */
    static bool isLeapYear(int year);

    /**
     * Returns the number of days in a given month for a given year.
     * Returns 0 if the month is invalid.
     */
    static int daysInMonth(int year, int month);

    /**
     * Converts ISO date "YYYY-MM-DD" to days since civil epoch (1970-01-01).
     * Validates strict length (10), '-' delimiters, numeric digits, and valid calendar day/month/year.
     * Returns std::nullopt if the date string is invalid or not in strict YYYY-MM-DD format.
     */
    static std::optional<int> parseDateToDays(const std::string& dateStr);

    /**
     * Calculates the number of overlapping nights between two valid dates.
     * Returns 0 if either date fails validation or if start >= end.
     */
    static int calculateOverlapNights(const std::string& start, const std::string& end);

    /**
     * Validates that both check_in and check_out dates are in strict YYYY-MM-DD format,
     * represent valid calendar dates, and satisfy check_in < check_out.
     */
    static bool isValidReservationDates(const std::string& checkIn, const std::string& checkOut);

    /**
     * Audits a batch of reservations for inventory overlaps.
     * Groups reservations by physical room (room_id), sorts chronologically by check_in,
     * and sweeps for collisions under the semi-open hotel convention [check_in, check_out).
     */
    static AuditResult detectOverlaps(const std::vector<Reservation>& reservations);
};

#endif // OVERLAP_DETECTOR_HPP
