/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : verify_results.cpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  Compares two simulation result files and computes a numerical
 *  similarity metric.
 *
 *  The verifier performs:
 *
 *      - Header validation
 *      - Node name verification
 *      - Numerical result comparison
 *      - Relative error estimation
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *
 *      Reference Simulation
 *                ↓
 *          CSV Result File
 *                ↓
 *              Verifier
 *                ↑
 *          CSV Result File
 *                ↑
 *        Generated Result
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *
 *  The verifier computes a normalized squared error:
 *
 *      error = Σ(xref - xtest)² / Σ(xref²)
 *
 *  A percentage-based similarity value is then reported:
 *
 *      match = (1 - error) · 100 %
 *
 *  Design Notes
 *  --------------------------------------------------------------------------
 *
 *  The simulator output starts with a fixed-size log section.
 *  These lines are skipped before comparing the numerical result data.
 *
 *  The comparison intentionally verifies node ordering and naming before
 *  evaluating numerical values. This guarantees that matrix reordering
 *  bugs or export inconsistencies are detected immediately.
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

/**
 * @brief Splits a CSV line into individual columns.
 *
 * Trailing carriage return characters originating from Windows-style
 * line endings are removed automatically.
 *
 * @param line CSV line.
 *
 * @return Parsed column tokens.
 */
std::vector<std::string> parseCsvLine(const std::string& line)
{
    std::vector<std::string> tokens;

    std::stringstream stream(line);
    std::string token;

    while (std::getline(stream, token, ','))
    {
        if (!token.empty() && token.back() == '\r')
        {
            token.pop_back();
        }

        tokens.push_back(token);
    }

    return tokens;
}

/**
 * @brief Skips a specified number of lines inside a file.
 *
 * Used to skip simulator log output before the numerical result section
 * begins.
 *
 * @param file Input file stream.
 * @param count Number of lines to skip.
 */
void skipLines(std::ifstream& file, int count)
{
    std::string line;

    for (int i = 0; i < count; ++i)
    {
        if (!std::getline(file, line))
        {
            std::cerr
                << "Error: File contains fewer than "
                << count
                << " lines."
                << std::endl;

            std::exit(EXIT_FAILURE);
        }
    }
}

/**
 * @brief Entry point of the result verifier.
 *
 * Expected command-line arguments:
 *
 *      argv[1] -> Reference CSV file
 *      argv[2] -> Test CSV file
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line arguments.
 *
 * @return Application exit code.
 */
int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        std::cerr
            << "Usage: "
            << argv[0]
            << " reference_file.csv test_file.csv"
            << std::endl;

        return EXIT_FAILURE;
    }

    std::ifstream reference_file(argv[1]);
    std::ifstream test_file(argv[2]);

    if (!reference_file.is_open() || !test_file.is_open())
    {
        std::cerr
            << "Failed to open one or more input files."
            << std::endl;

        return EXIT_FAILURE;
    }

    /*
     * Skip simulator log output.
     *
     * Numerical data begins after the fixed metadata section.
     */
    skipLines(reference_file, 12);
    skipLines(test_file, 12);

    /*
     * Read CSV header rows.
     */
    std::string reference_header_line;
    std::string test_header_line;

    if (!std::getline(reference_file, reference_header_line) ||
        !std::getline(test_file, test_header_line))
    {
        std::cerr
            << "Failed to read CSV headers."
            << std::endl;

        return EXIT_FAILURE;
    }

    std::vector<std::string> reference_headers =
        parseCsvLine(reference_header_line);

    std::vector<std::string> test_headers =
        parseCsvLine(test_header_line);

    /*
     * Verify the number of exported columns.
     */
    if (reference_headers.size() != test_headers.size())
    {
        std::cerr
            << "Verification failed: Different column counts."
            << std::endl;

        std::cerr
            << "Reference file: "
            << reference_headers.size()
            << " columns, Test file: "
            << test_headers.size()
            << " columns."
            << std::endl;

        return EXIT_FAILURE;
    }

    /*
     * Verify node ordering and naming.
     */
    bool header_mismatch_detected = false;

    for (size_t column = 0;
         column < reference_headers.size();
         ++column)
    {
        if (reference_headers[column] != test_headers[column])
        {
            std::cerr
                << "Verification failed: Header mismatch in column "
                << column + 1
                << std::endl;

            std::cerr
                << "Reference: "
                << reference_headers[column]
                << " | Test: "
                << test_headers[column]
                << std::endl;

            header_mismatch_detected = true;
        }
    }

    if (header_mismatch_detected)
    {
        return EXIT_FAILURE;
    }

    /*
     * Compare all numerical simulation data.
     */
    double sum_squared_error = 0.0;
    double sum_squared_reference = 0.0;

    std::string reference_line;
    std::string test_line;

    while (std::getline(reference_file, reference_line) &&
           std::getline(test_file, test_line))
    {
        const auto reference_row =
            parseCsvLine(reference_line);

        const auto test_row =
            parseCsvLine(test_line);

        /*
         * Ignore incomplete trailing lines.
         */
        if (reference_row.size() != reference_headers.size() ||
            test_row.size() != test_headers.size())
        {
            continue;
        }

        for (size_t column = 0;
             column < reference_row.size();
             ++column)
        {
            const double reference_value =
                std::stod(reference_row[column]);

            const double test_value =
                std::stod(test_row[column]);

            const double difference =
                reference_value - test_value;

            sum_squared_error +=
                difference * difference;

            sum_squared_reference +=
                reference_value * reference_value;
        }
    }

    /*
     * Prevent division by zero.
     */
    if (sum_squared_reference == 0.0)
    {
        std::cerr
            << "Verification failed: Reference data contains only zeros."
            << std::endl;

        return EXIT_FAILURE;
    }

    const double error_ratio =
        sum_squared_error / sum_squared_reference;

    const double match_percentage =
        (1.0 - error_ratio) * 100.0;

    const double error_percentage =
        error_ratio * 100.0;

    std::cout << "----------------------------------------\n";
    std::cout << "Verification completed successfully.\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Relative error: " << error_percentage << " %\n";
    std::cout << "Match score:    " << match_percentage << " %\n\n";

    return EXIT_SUCCESS;
}