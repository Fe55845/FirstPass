/**
* ============================================================================
* Project : FirstPass
* File : generate_tran_testbench.cpp
*
* Description
* --------------------------------------------------------------------------
* Generates a deterministic transient analysis testbench used for
* validating the parser, transient analysis backend and time-domain
* simulation workflow.
*
* The generated netlist combines:
*
* - Resistors
* - Capacitors
* - Inductors
* - Independent voltage sources
*
* Application Workflow
* --------------------------------------------------------------------------
*
* Generated Netlist
* ↓
* Parser
* ↓
* Netlist
* ↓
* Transient Analysis
* ↓
* CSV Export
*
* Test Coverage
* --------------------------------------------------------------------------
*
* - Node indexing and reverse lookup
* - Ground node handling
* - Component classification
* - Transient MNA matrix assembly
* - State-space conversion
* - Differential-algebraic equation handling
* - Voltage source parsing
* - Engineering unit parsing
* - Capacitive element stamping
* - Inductive element stamping
* - Moderate-sized RLC network generation
*
* Design Notes
* --------------------------------------------------------------------------
*
* The generated network is not intended to model a specific physical
* circuit. Instead, it creates a deterministic collection of passive
* components that exercises the transient MNA builder, state-space
* conversion and numerical integration workflow under a variety of
* connectivity patterns.
*
* Author : Felix Zenhäusern
* Created : 2026
* License : MIT License
* ============================================================================
*/

#include <cstdint>
#include <fstream>

/**
 * Number of generated network columns.
 */
const uint32_t num_of_columns = 10;

int main()
{
    std::ofstream netlist_file("Tran_Testbench.net");

    netlist_file << "* Tran Analysis Testbench *\n";

    /*
     * Component numbering for the automatically generated RLC network.
     */
    uint32_t next_resistor_index  = 1;
    uint32_t next_capacitor_index = 1;
    uint32_t next_inductor_index  = 1;

    netlist_file << "V1 N0 0 1\n\n";

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
    for (uint32_t column = 0; column < num_of_columns - 1; ++column)
    {
        /*
            * Horizontal resistor branch.
            */
        netlist_file
            << "R"
            << next_resistor_index
            << " "
            << "N" << 2 * column
            << " N" << 2 * column + 1
            << " 1k\n";

        next_resistor_index++;

        /*
        * Horizontal inductor branch.
        */
        netlist_file
            << "L"
            << next_inductor_index
            << " "
            << "N" << 2 * column + 1
            << " N" << 2 * column + 2
            << " 1m\n";

        next_inductor_index++;

        /*
        * Vertical capacitor branch.
        */
        netlist_file
            << "C"
            << next_capacitor_index
            << " "
            << "N" << 2 * column + 2
            << " 0"
            << " 1u\n";

        next_capacitor_index++;
    }

    /*
     * ------------------------------------------------------------------------
     * Transient Simulation Configuration
     * ------------------------------------------------------------------------
     *
     * Perform a Transient Simulation:
     *
     *      Start Time : 0 s
     *      End Time   : 15 s
     *      Resolution : adaptive
     */
    netlist_file << "\n.tran 0 0.3 0 startup\n";

    /*
     * Request magnitude and phase export of a representative node.
     */
    netlist_file << ".print tran V(N" << num_of_columns*2-2 << ")\n";

    netlist_file << ".end\n";

    return 0;
}
