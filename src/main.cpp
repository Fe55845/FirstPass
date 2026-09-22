/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : main.cpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  Main entry point of the simulator.
 *
 *  This file is intentionally kept lightweight and only performs:
 *
 *      1. Command-line validation
 *      2. Simulation environment setup
 *      3. Netlist parsing
 *      4. Simulation execution
 *      5. Result export finalization
 *
 *  The actual simulation logic is implemented inside the SimulationRunner
 *  class and the associated analysis backends.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *
 *      Input Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *    SimulationRunner
 *            ↓
 *      OP / AC / TRAN
 *            ↓
 *      Result Export
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *
 *  The simulator is based on Modified Nodal Analysis (MNA).
 *
 *  Depending on the selected analysis type, one of the following equation
 *  systems is solved:
 *
 *      Operating Point:
 *
 *          Gx = b
 *
 *      AC Analysis:
 *
 *          G(jω)x = b
 *
 *      Transient Analysis:
 *
 *          Cx' + Gx = b
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#include <chrono>
#include <filesystem>
#include <iostream>
#include <string>

// Core project modules
#include "parser.hpp"
#include "simulation_results.hpp"
#include "result_exporter.hpp"
#include "simulation_runner.hpp"

/**
 * Current simulator version.
 */
const std::string VERSION = "FirstPass 0.1";

/**
 * @brief Application entry point.
 *
 * Expected command-line arguments:
 *
 *      argv[1] -> Input SPICE netlist
 *      argv[2] -> Output CSV file
 *
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument list.
 *
 * @return Process exit status.
 *
 *      0 -> Success
 *      1 -> Configuration or input error
 */
int main(int argc, char* argv[])
{
    /*
     * The simulator requires exactly two user-supplied arguments:
     *
     *      1. Input netlist
     *      2. Output file
     *
     * If the argument count is incorrect, display the integrated help page.
     */
    if (argc != 3)
    {
        std::cout
            << "FirstPass - Linear Circuit Simulator\n"
            << "====================================\n\n"

            << "Description\n"
            << "-----------\n"
            << "FirstPass is a SPICE-like circuit simulator focused on\n"
            << "linear electrical networks. The simulator supports\n"
            << "Operating Point, AC and Transient analysis using\n"
            << "Modified Nodal Analysis (MNA).\n\n"

            << "Usage\n"
            << "-----\n"
            << "FirstPass <input_netlist.cir> <output_file.csv>\n\n"

            << "Arguments\n"
            << "---------\n"
            << "input_netlist.cir    SPICE netlist describing the circuit\n"
            << "output_file.csv      Simulation results and log output\n\n"

            << "Example\n"
            << "-------\n"
            << "FirstPass examples/rc_lowpass.cir results.csv\n\n"

            << "Supported Analyses\n"
            << "------------------\n"
            << "  .op    Operating Point Analysis\n"
            << "  .ac    Small Signal AC Analysis\n"
            << "  .tran  Transient Analysis\n\n"

            << "Project Status\n"
            << "--------------\n"
            << "Research and educational project.\n"
            << "Some SPICE features may not yet be implemented.\n\n";

        return 1;
    }

    const std::string input_file  = argv[1];
    const std::string output_file = argv[2];

    /*
     * Print general application and simulation information.
     *
     * This information simplifies debugging and allows simulation reports
     * to be associated with a specific software version.
     */
    std::cout
        << "******\n"
        << "** " << VERSION << " : Fast Simulation of Linear Circuits\n"
        << "** Author: Felix Zenhäusern\n"
        << "** License: MIT License\n"
        << "** Input File: " << input_file << "\n"
        << "** Output File: " << output_file << "\n"
        << "******\n"
        << std::endl;

    /*
     * Store metadata and timing information generated during execution.
     */
    SimulationResults simulation_result;

    /*
     * Record the wall-clock start time.
     *
     * The timestamp is later used for reporting and performance analysis.
     */
    simulation_result.start_time =
        std::chrono::high_resolution_clock::now();

    /*
     * Store the canonical location of the simulated circuit.
     *
     * Using absolute paths avoids ambiguities when reports are generated
     * from different working directories.
     */
    simulation_result.input_path =
        std::filesystem::absolute(input_file);

    /*
     * Verify that the specified simulation file exists before allocating
     * any analysis structures.
     */
    if (!std::filesystem::exists(simulation_result.input_path))
    {
        std::cout
            << "Input netlist does not exist. "
            << "Please verify the supplied file path."
            << std::endl;

        return 1;
    }

    /*
     * Central data structure containing the complete circuit description
     * and simulation configuration.
     */
    Netlist netlist;

    /*
     * Result exporter responsible for CSV generation and simulation logs.
     */
    ResultExporter output(
        output_file,
        &netlist,
        &simulation_result
    );

    /*
     * Convert the textual SPICE description into the internal netlist
     * representation used throughout the simulator.
     */
    Parser parser(
        input_file,
        &netlist
    );

    /*
     * Abort execution if the parser did not discover any supported
     * circuit components.
     *
     * Running matrix assembly and solver stages on an empty circuit
     * would not produce meaningful results.
     */
    if (netlist.num_resistors == 0 &&
        netlist.num_current_sources == 0 &&
        netlist.num_voltage_sources == 0 &&
        netlist.num_capacitors == 0 &&
        netlist.num_inductors == 0)
    {
        std::cout
            << "The selected netlist does not contain any supported "
            << "components."
            << std::endl;

        return 1;
    }

    /*
     * Execute the complete simulation workflow.
     *
     * The SimulationRunner automatically selects the appropriate
     * analysis backend based on the parsed simulation directive.
     */
    SimulationRunner simulation_runner(
        &netlist,
        &simulation_result,
        &output
    );

    /*
    * Flush all remaining buffered data and close the output file.
    */
    output.output.close();

    /*
     * Export collected simulation metadata and statistics.
     */
    output.print_logs();

    return 0;
}