/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : generate_op_testbench.cpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  Generates a deterministic operating-point testbench used for validating
 *  the parser and operating point analysis workflow.
 *
 *  The generated netlist combines:
 *
 *      - Resistors
 *      - Capacitors
 *      - Inductors
 *      - Independent current sources
 *      - Independent voltage sources
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
 *        OP Analysis
 *              ↓
 *          CSV Export
 *
 *  Test Coverage
 *  --------------------------------------------------------------------------
 *
 *      - Node indexing and reverse lookup
 *      - Ground node handling
 *      - Component classification
 *      - Operating point matrix assembly
 *      - Voltage source syntax variations
 *      - Engineering unit parsing
 *      - Moderate-sized resistor network generation
 *
 *  Design Notes
 *  --------------------------------------------------------------------------
 *
 *  The generated resistor grid is not intended to model a specific circuit.
 *  Instead, it provides a predictable connectivity pattern that increases
 *  matrix size and exercises the MNA construction logic.
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#include <cstdint>
#include <fstream>

/**
 * Number of resistor-network rows.
 */
const uint32_t num_of_rows = 1000;

/**
 * Number of resistor-network columns.
 */
const uint32_t num_of_columns = 1000;

int main()
{
    std::ofstream netlist("OP_Testbench.net");

    /*
     * ------------------------------------------------------------------------
     * Basic Parser and OP Analysis Verification
     * ------------------------------------------------------------------------
     *
     * Tests:
     *
     *      - Voltage sources
     *      - Current sources
     *      - Resistors
     *      - Capacitors
     *      - Inductors
     *      - Ground node handling
     *      - Explicit and implicit DC source syntax
     *      - Common engineering suffixes
     */
    netlist
        << "* OP Analysis Testbench *\n"
        << "V1 0 NV1 DC 1k\n"
        << "V2 NV1 N0 1\n"
        << "I1 0 NI1 1m\n"
        << "R1 NI1 N0 1k\n"
        << "R2 N0 NC1 1u\n"
        << "C1 0 NC1 1k\n"
        << "L1 NC1 NR2 1m\n"
        << "R3 0 NR2 100k\n\n";

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
     */
    netlist
        << "V3 0 NV3 1k\n"
        << "R4 NV3 NR4 1G\n"
        << "R5 NR4 NR5 1Meg\n"
        << "R6 NR5 NR6 1k\n"
        << "R7 NR6 NR7 1\n"
        << "R8 NR7 NR8 1m\n"
        << "R9 NR8 NR9 1u\n"
        << "R10 NR9 NR10 1n\n"
        << "R11 NR10 NR11 1p\n"
        << "R12 NR11 0 1000f\n\n";

    /*
     * First automatically generated resistor index.
     */
    uint32_t Resistor_index = 13;

    /*
     * ------------------------------------------------------------------------
     * Resistor Grid Generation
     * ------------------------------------------------------------------------
     *
     * Generate a rectangular resistor mesh in order to exercise:
     *
     *      - Node creation
     *      - Node lookup
     *      - Matrix assembly
     *      - Parser performance
     */
    for (uint32_t row = 0; row < num_of_rows - 1; ++row)
    {
        for (uint32_t column = 0; column < num_of_columns - 1; ++column)
        {
            netlist
                << "R"
                << Resistor_index
                << " "
                << "N" << row * num_of_columns + column
                << " N" << row * num_of_columns + column + 1
                << " 10\n";

            Resistor_index++;
            
            netlist
                << "R"
                << Resistor_index
                << " "
                << "N" << row * num_of_columns + column
                << " N" << (row + 1) * num_of_columns + column
                << " 10\n";
            
            Resistor_index++;
        }

        netlist
            << "R"
            << Resistor_index
            << " "
            << "N" << row * num_of_columns + num_of_columns - 1
            << " N" << (row + 1) * num_of_columns + num_of_columns - 1
            << " 10\n";
        Resistor_index ++;
    }

    /*
     * Connect the resistor mesh to ground.
     */
    netlist
        << "\nR"
        << Resistor_index
        << " "
        << "N" << (num_of_rows - 1) * num_of_columns
        << " 0"
        << " 10\n";

    Resistor_index++;

    /*
     * Generate the final horizontal resistor chain in the bottom row.
     */
    for (uint32_t column = 0; column < num_of_columns - 1; ++column)
    {
        netlist
            << "R"
            << Resistor_index
            << " "
            << "N" << (num_of_rows - 1) * num_of_columns + column
            << " N" << (num_of_rows - 1) * num_of_columns + column + 1
            << " 10\n";
        
        Resistor_index++;
    }

    /*
     * Request operating point analysis.
     */
    netlist << "\n.op\n";
    netlist << ".end\n";

    return 0;
}
