/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : simulation_results.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the SimulationResults class, which serves as a lightweight
 *  container for execution metadata and runtime timing information.
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
 *    SimulationResults  <-- [This File]
 *            ↓
 *    ResultExporter
 *
 *  Design Notes
 *  --------------------------------------------------------------------------
 *  To optimize memory footprint and runtime execution performance, large-scale
 *  simulation result vectors (e.g., time-series data or node voltage matrices)
 *  are streamed or handled directly by the processing pipelines rather than
 *  being buffered within this container.
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <chrono>
#include <filesystem>

/**
 * @brief Container class for simulation runtime metadata and execution tracking.
 *
 * The SimulationResults structure holds high-level execution metadata, such as
 * the input netlist file path and the initial execution timestamp used for
 * performance logging.
 */
class SimulationResults {
public:
    /**
     * Path to the processed SPICE netlist input file.
     */
    std::filesystem::path input_path;

    /**
     * Timestamp marking the start time of the simulation execution.
     */
    std::chrono::system_clock::time_point start_time;

    /**
     * @brief Constructs a new SimulationResults instance.
     *
     * Captures the current system time as the simulation start timestamp.
     */
    SimulationResults() : start_time(std::chrono::system_clock::now()) {}

    /**
     * @brief Default destructor.
     */
    ~SimulationResults() = default;
};