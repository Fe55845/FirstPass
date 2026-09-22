#pragma once

/**
 * ============================================================================
 *  Project        : Linear Circuit Simulator
 *  File           : netlist.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the core data structures used throughout the simulator.
 *
 *  The parser converts a textual SPICE netlist into the internal data model
 *  defined in this file. Every simulation stage operates on these structures:
 *
 *      Parser
 *          ↓
 *      Netlist
 *          ↓
 *      MNA Builder
 *          ↓
 *      Solver
 *          ↓
 *      Result Export
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *  SPICE netlists use textual node names:
 *
 *      R1 vin vout 1k
 *      C1 vout 0 10u
 *
 *  Matrix-based simulation algorithms operate much more efficiently on
 *  integer indices than on strings. Therefore, all node names are translated
 *  into consecutive integer identifiers before any matrix assembly starts.
 *
 *  Example:
 *
 *      "0"     -> -1    (ground node)
 *      "vin"   -> 0
 *      "vout"  -> 1
 *
 *  The resulting Netlist object contains the complete circuit description and
 *  all simulation configuration parameters required by the simulator.
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

// Include Componets
#include "independent_voltage_source.hpp"
#include "passive_component.hpp"

/**
 * @brief Defines the requested simulation type.
 *
 * The parser extracts the simulation mode from SPICE directives such as:
 *
 *      .op
 *      .ac
 *      .tran
 *
 * The main program later selects the appropriate solver backend.
 */
enum class Analysis : uint8_t {
    tran = 0,   ///< Time-domain transient simulation
    ac   = 1,   ///< Small-signal frequency domain analysis
    dc   = 2,   ///< Reserved for future DC sweep implementation
    op   = 3    ///< Operating point analysis
};

/**
 * @brief Defines the frequency sweep mode used during AC analysis.
 *
 * These modes correspond to the standard SPICE sweep options.
 */
enum class AcAnalysis : uint8_t {
    oct  = 0,   ///< Sweep with a specified number of points per octave
    dec  = 1,   ///< Sweep with a specified number of points per decade
    lin  = 2,   ///< Linear frequency sweep
    list = 3    ///< Explicit list of frequencies
};

/**
 * @brief Stores the complete parsed circuit and simulation configuration.
 *
 * This is the central data structure exchanged between all modules of the
 * simulator.
 *
 * After parsing, this object contains:
 *
 *      - Circuit topology
 *      - Component values
 *      - Voltage sources
 *      - Node mappings
 *      - Simulation settings
 *      - Solver selection hints
 */
class Netlist {
public:

    /*
     * =====================================================================
     * Node Translation Tables
     * =====================================================================
     *
     * SPICE uses string-based node names.
     *
     * Matrix solvers operate on integer indices.
     *
     * These lookup tables provide efficient conversion in both directions.
     */

    /**
     * Constant representing the ground reference node index (-1).
     */
    static constexpr int32_t GROUND_NODE = -1;

    /**
     * Maps textual node names to internal integer indices.
     *
     * Example:
     *
     *      "vin"  -> 0
     *      "vout" -> 1
     */
    std::unordered_map<std::string, int32_t> node_name_to_index;

    /**
     * Reverse lookup table.
     *
     * Primarily used during result export when node voltages are written back
     * using their original SPICE names.
     */
    std::unordered_map<int32_t, std::string> index_to_node_name;

    /*
     * =====================================================================
     * Circuit Components
     * =====================================================================
     */

    /**
     * Collection of all passive two-terminal components.
     */
    std::vector<PassiveComponent> passive_components;

    /**
     * Collection of all independent voltage sources.
     */
    std::vector<IndependentVoltageSource> voltage_sources;

    /*
     * =====================================================================
     * Circuit Statistics
     * =====================================================================
     *
     * These counters are generated during parsing and later used to:
     *
     *      - Estimate matrix sparsity
     *      - Select solver implementations
     *      - Allocate matrices
     *      - Generate simulation reports
     */

    /**
     * Number of non-ground circuit nodes.
     */
    uint32_t num_nodes;

    uint32_t num_resistors;
    uint32_t num_inductors;
    uint32_t num_capacitors;
    uint32_t num_voltage_sources;
    uint32_t num_current_sources;

    /*
     * =====================================================================
     * Matrix Dimensions
     * =====================================================================
     */

    /**
     * Order of the reduced state-space representation.
     *
     * This value is determined during the state-space conversion process and
     * corresponds to the number of dynamic state variables.
     */
    uint32_t order_state_space;

    /**
     * Order of the Modified Nodal Analysis equation system.
     *
     * Example:
     *
     *      Number of Nodes
     *    + Number of Voltage Sources
     *    + Number of Inductor Currents
     *
     * depending on the selected simulation mode.
     */
    uint32_t order_mna_equation;

    /*
     * =====================================================================
     * Solver Configuration
     * =====================================================================
     */

    /**
     * Selects whether the simulator should use a dense matrix backend.
     *
     * The value is determined automatically using a simple sparsity estimate.
     */
    bool use_dense_mna;

    /**
     * Indicates whether the MNA system is purely real-valued.
     *
     * Frequency-domain simulations generally require complex-valued matrices,
     * while operating point simulations remain real-valued.
     */
    bool mna_is_real;

    /*
     * =====================================================================
     * Analysis Configuration
     * =====================================================================
     */

    /**
     * Selected simulation mode.
     */
    Analysis analysis;

    /*
     * =====================================================================
     * Transient Analysis Parameters
     * =====================================================================
     */

    /**
     * Simulation start time in seconds.
     */
    double t_start;

    /**
     * Simulation stop time in seconds.
     */
    double t_end;

    /**
     * Sampling interval used for output generation.
     */
    double dt_sample;

    /*
     * =====================================================================
     * AC Analysis Parameters
     * =====================================================================
     */

    /**
     * Selected AC frequency sweep type.
     */
    AcAnalysis ac_analysis;

    /**
     * Start frequency of the sweep in Hertz.
     */
    double f_start;

    /**
     * End frequency of the sweep in Hertz.
     */
    double f_end;

    /**
     * Explicit frequency list for LIST-based AC analysis.
     */
    std::vector<double> f_list;

    /**
     * Number of simulation points.
     *
     * Depending on the selected analysis this value may represent:
     *
     *      - Number of transient samples
     *      - Points per decade
     *      - Points per octave
     *      - Number of linear sweep points
     */
    uint32_t n = 1000;

    /**
     * @brief Default destructor.
     *
     * STL containers automatically release all allocated memory, therefore no
     * explicit cleanup is required.
     */
    ~Netlist() = default;
};