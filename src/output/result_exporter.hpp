/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : result_exporter.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the ResultExporter class, which handles formatting,
 *  logging, and writing simulation output data to target files (e.g., CSV).
 *  It includes header pre-allocation for metadata and timing metrics.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *    SimulationRunner
 *            ↓
 *    ResultExporter     <-- [This File]
 *
 *  Design Notes
 *  --------------------------------------------------------------------------
 *  To prevent file re-allocation when logging metadata after simulation execution,
 *  a fixed-size header section (HEADER_SIZE) is pre-allocated at file creation.
 *  The header space is subsequently overwritten with execution details, runtime
 *  metrics, and circuit specifications.
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <chrono>
#include <ctime>
#include <fstream>
#include <map>
#include <string>

#include "netlist.hpp"
#include "simulation_results.hpp"

/**
 * @brief Handles formatting and writing simulation results and logs to file.
 *
 * The ResultExporter manages output file creation, reserves space for header logs,
 * and formats performance metrics (e.g., simulation runtime, circuit stats)
 * alongside numerical simulation data.
 */
class ResultExporter {
private:
    /**
     * Path to the destination output file.
     */
    std::string outputfile;

    /**
     * Pointer to the simulation results data container.
     */
    SimulationResults* simulation_result;

    /**
     * Pointer to the circuit netlist containing node mapping and circuit topology.
     */
    Netlist* netlist;

    /**
     * Size in bytes reserved at the beginning of the output file for header metrics.
     */
    size_t HEADER_SIZE = 50 * 11;

public:
    /**
     * Binary file stream handle for output operations.
     */
    std::ofstream output;

    /**
     * @brief Constructs the exporter, opens the output file, and reserves header space.
     *
     * @param outputfile Path to the target output file.
     * @param netlist Pointer to the Netlist structure containing component counts.
     * @param simulation_result Pointer to the SimulationResults structure.
     */
    ResultExporter(std::string outputfile, Netlist* netlist, SimulationResults* simulation_result)
        : outputfile(outputfile), netlist(netlist), simulation_result(simulation_result) {

        output.open(outputfile, std::ios::binary);

        // Pre-allocate header block space
        std::string dummyHeader(HEADER_SIZE, ' ');
        dummyHeader.back() = '\n';
        output << dummyHeader;
    }

    /**
     * @brief Overwrites the pre-allocated header region with simulation run logs.
     *
     * Seeks to the start of the output file and writes execution details such as
     * the netlist input path, start timestamp, total simulation runtime, MNA matrix order,
     * and component breakdown counters.
     */
    void print_logs() {
        output.open(outputfile, std::ios::in | std::ios::out | std::ios::binary);
        output.seekp(0, std::ios::beg);

        // Write circuit source path
        output << "Circuit: " << simulation_result->input_path << "\n";

        // Format start timestamp
        std::time_t jetzt_time_t = std::chrono::system_clock::to_time_t(simulation_result->start_time);
        std::tm* lokal_zeit      = std::localtime(&jetzt_time_t);
        char puffer[80];
        std::strftime(puffer, sizeof(puffer), "%d.%m.%Y %H:%M:%S", lokal_zeit);
        output << "Start Time: " << puffer << "\n";

        // Calculate runtime duration
        auto end      = std::chrono::system_clock::now();
        auto duration = end - simulation_result->start_time;

        auto hrs  = std::chrono::duration_cast<std::chrono::hours>(duration);
        auto mins = std::chrono::duration_cast<std::chrono::minutes>(duration % std::chrono::hours(1));
        auto secs = std::chrono::duration_cast<std::chrono::seconds>(duration % std::chrono::minutes(1));
        auto ms   = std::chrono::duration_cast<std::chrono::milliseconds>(duration % std::chrono::seconds(1));

        output << "Simulation Time: "
               << hrs.count()  << "h "
               << mins.count() << "min "
               << secs.count() << "s "
               << ms.count()   << "ms\n\n";

        // Write system matrix & component statistics
        output << "Order of MNA: "              << netlist->order_mna_equation << "\n";
        output << "Number of Voltage Sources: " << netlist->num_voltage_sources << "\n";
        output << "Number of Current Sources: " << netlist->num_current_sources << "\n";
        output << "Number of Resistors: "       << netlist->num_resistors       << "\n";
        output << "Number of Inductors: "       << netlist->num_inductors       << "\n";
        output << "Number of Capacitors: "      << netlist->num_capacitors      << "\n\n";
    }

    /**
     * @brief Default destructor.
     */
    ~ResultExporter() = default;
};