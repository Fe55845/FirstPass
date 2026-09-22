/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : generate_ac_testbench.cpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  Generates a deterministic AC analysis testbench used for validating
 *  the parser, AC analysis backend and frequency-domain simulation flow.
 *
 *  The generated netlist combines:
 *
 *      - Resistors
 *      - Capacitors
 *      - Inductors
 *      - Independent AC voltage sources
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *
 *      Generated Netlist
 *              ↓
 *            Parser
 *              ↓
 *           Netlist
 *              ↓
 *         AC Analysis
 *              ↓
 *          CSV Export
 *
 *  Test Coverage
 *  --------------------------------------------------------------------------
 *
 *      - Node indexing and reverse lookup
 *      - Ground node handling
 *      - Component classification
 *      - Complex-valued MNA matrix assembly
 *      - AC voltage source parsing
 *      - Engineering unit parsing
 *      - Capacitive element stamping
 *      - Inductive element stamping
 *      - Moderate-sized RLC network generation
 *
 *  Design Notes
 *  --------------------------------------------------------------------------
 *
 *  The generated network is not intended to model a specific physical
 *  circuit. Instead, it creates a deterministic collection of passive
 *  components that exercises the AC matrix builder and solver under
 *  a variety of connectivity patterns.
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#include <cstdint>
#include <fstream>

/**
 * Number of generated network rows.
 */
const uint32_t num_of_rows = 100;

/**
 * Number of generated network columns.
 */
const uint32_t num_of_columns = 100;

int main()
{
    std::ofstream netlist_file("AC_Testbench.net");

    /*
     * ------------------------------------------------------------------------
     * Basic Parser and AC Analysis Verification
     * ------------------------------------------------------------------------
     *
     * Tests:
     *
     *      - AC voltage sources
     *      - Resistors
     *      - Capacitors
     *      - Inductors
     *      - Ground node handling
     *      - Basic RLC behaviour
     *      - Node indexing
     */
    netlist_file
        << "* AC Analysis Testbench *\n"
        << "V1 0 NV1 AC 1k 0\n"
        << "R1 NV1 NC1 1k\n"
        << "C1 0 NC1 1k\n"
        << "L1 NC1 NR2 1m\n"
        << "R2 0 NR2 100k\n\n";

    /*
     * ------------------------------------------------------------------------
     * Engineering Prefix Verification
     * ------------------------------------------------------------------------
     *
     * Tests all currently supported engineering prefixes:
     *
     *      G    -> 1e9
     *      Meg  -> 1e6
     *      k    -> 1e3
     *      1    -> 1
     *      m    -> 1e-3
     *      u    -> 1e-6
     *      n    -> 1e-9
     *      p    -> 1e-12
     *      f    -> 1e-15
     *
     * Capacitor chain:
     *
     *      1m -> 1u -> 1n -> 1p -> 1000f
     */
    netlist_file
        << "V3 0 NV3 AC 1k 0\n"
        << "R3 NV3 NR3 1k\n"
        << "C2 NR3 NC2 1m\n"
        << "C3 NC2 NC3 1u\n"
        << "C4 NC3 NC4 1n\n"
        << "C5 NC4 NC5 1p\n"
        << "C6 NC5 0 1000f\n\n";

    /*
     * Inductor chain:
     *
     *      1m -> 1u -> 1n -> 1p -> 1000f
     */
    netlist_file
        << "V4 0 NV4 AC 1k 0\n"
        << "R4 NV4 NR4 1k\n"
        << "L2 NR4 NL2 1m\n"
        << "L3 NL2 NL3 1u\n"
        << "L4 NL3 NL4 1n\n"
        << "L5 NL4 NL5 1p\n"
        << "L6 NL5 0 1000f\n\n";

    /*
     * Component numbering for the automatically generated RLC network.
     */
    uint32_t next_resistor_index  = 5;
    uint32_t next_capacitor_index = 7;
    uint32_t next_inductor_index  = 7;

    netlist_file << "V2 0 N0 AC 1 0\n";

    /*
     * ------------------------------------------------------------------------
     * RLC Mesh Generation
     * ------------------------------------------------------------------------
     *
     * Generates a mixed resistor, capacitor and inductor network used to
     * exercise:
     *
     *      - Node creation
     *      - Node lookup
     *      - Complex matrix assembly
     *      - Component stamping
     *      - Solver scalability
     */
    for (uint32_t row = 0; row < num_of_rows - 1; ++row)
    {
        for (uint32_t column = 0; column < num_of_columns - 1; ++column)
        {
            /*
             * Horizontal resistor branch.
             */
            netlist_file
                << "R"
                << next_resistor_index
                << " "
                << "N" << 2 * (row * num_of_columns + column)
                << " N" << 2 * (row * num_of_columns + column) + 1
                << " 10Meg\n";

            next_resistor_index++;

            /*
             * Horizontal inductor branch.
             */
            netlist_file
                << "L"
                << next_inductor_index
                << " "
                << "N" << 2 * (row * num_of_columns + column) + 1
                << " N" << 2 * (row * num_of_columns + column) + 2
                << " 10n\n";

            next_inductor_index++;

            /*
             * Vertical capacitor branch.
             */
            netlist_file
                << "C"
                << next_capacitor_index
                << " "
                << "N" << 2 * (row * num_of_columns + column)
                << " N" << 2 * ((row + 1) * num_of_columns + column)
                << " 10n\n";

            next_capacitor_index++;
        }

        /*
         * Capacitor branch on the right edge of the mesh.
         */
        netlist_file
            << "C"
            << next_capacitor_index
            << " "
            << "N" << 2 * (row * num_of_columns + num_of_columns - 1)
            << " N" << 2 * ((row + 1) * num_of_columns + num_of_columns - 1)
            << " 10n\n";

        next_capacitor_index++;
    }

    /*
     * Connect the generated network to ground.
     */
    netlist_file
        << "\nR"
        << next_resistor_index
        << " "
        << "N" << 2 * ((num_of_rows - 1) * num_of_columns)
        << " 0"
        << " 10\n";

    next_resistor_index++;

    /*
     * Complete the final horizontal resistor chain in the bottom row.
     */
    for (uint32_t column = 0; column < num_of_columns - 1; ++column)
    {
        netlist_file
            << "R"
            << next_resistor_index
            << " "
            << "N" << 2 * ((num_of_rows - 1) * num_of_columns + column)
            << " N" << 2 * ((num_of_rows - 1) * num_of_columns + column + 1)
            << " 10\n";

        next_resistor_index++;
    }

    /*
     * ------------------------------------------------------------------------
     * AC Frequency Sweep Configuration
     * ------------------------------------------------------------------------
     *
     * Perform a logarithmic decade sweep:
     *
     *      Start Frequency : 10k Hz
     *      End Frequency   : 1 GHz
     *      Resolution      : 1000 points per decade
     */
    netlist_file << "\n.ac dec 100 10k 1000Meg\n";

    /*
     * Request magnitude and phase export of a representative node.
     */
    netlist_file
        << ".print ac VM(N" << num_of_rows
        << ") VP(N" << num_of_rows
        << ")\n";

    netlist_file << ".end\n";

    return 0;
}
