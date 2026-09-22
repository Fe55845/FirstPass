/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : parser.cpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file implements the parsing functions defined in the Parser class.
 *  It provides high-performance conversion of SPICE numerical value suffixes,
 *  parses SPICE analysis directives (.tran, .ac), and handles independent
 *  voltage source configurations.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser        <-- [This File Implementation]
 *            ↓
 *        Netlist
 *            ↓
 *    SimulationRunner
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#include "parser.hpp"

/**
 * @brief Parses a numerical value with optional SPICE unit prefixes.
 *
 * Uses std::from_chars for zero-allocation fast string-to-double conversion,
 * followed by character evaluation for scale prefixes (e.g., 'k', 'u', 'Meg').
 *
 * @param sv String view containing the numerical value and suffix.
 * @return Evaluated double value including scale factor.
 */
double Parser::parseSpiceValue(std::string_view sv) {
    double base_val = 0.0;
    auto [ptr, ec] = std::from_chars(sv.data(), sv.data() + sv.size(), base_val);

    std::string_view prefix(ptr, (sv.data() + sv.size()) - ptr);
    if (prefix.empty()) {
        return base_val;
    }

    // SPICE-style multi-character prefix check (case-insensitive for 'Meg')
    if (prefix.rfind("Meg", 0) == 0 || prefix.rfind("meg", 0) == 0 || prefix.rfind("MEG", 0) == 0) {
        return base_val * 1e6;
    }

    // Single-character SPICE prefixes
    switch (prefix[0]) {
        case 'm': case 'M': return base_val * 1e-3;
        case 'u': case 'U': return base_val * 1e-6;
        case 'n': case 'N': return base_val * 1e-9;
        case 'p': case 'P': return base_val * 1e-12;
        case 'f': case 'F': return base_val * 1e-15;
        case 'k': case 'K': return base_val * 1e3;
        case 'g': case 'G': return base_val * 1e9;
        default:           return base_val;
    }
}

/**
 * @brief Parses the .tran simulation directive.
 *
 * Sets the analysis mode to transient analysis and extracts starting time,
 * ending time, and sample interval step sizes.
 *
 * @param line Raw line view of the directive.
 */
void Parser::parseTransientDirective(std::string_view line) {
    netlist->analysis = Analysis::tran;
    std::stringstream ss{std::string(line)};
    std::string opcond, start_time, end_time;

    ss >> opcond >> start_time >> end_time;

    netlist->t_start   = parseSpiceValue(start_time);
    netlist->t_end     = parseSpiceValue(end_time);
    netlist->dt_sample = (netlist->t_end - netlist->t_start) / netlist->n;
}

/**
 * @brief Parses the .ac simulation directive.
 *
 * Sets the analysis mode to AC analysis, determines sweep mode (oct, dec, lin, list),
 * and sets frequency bound limits.
 *
 * @param line Raw line view of the directive.
 */
void Parser::parseAcDirective(std::string_view line) {
    netlist->analysis = Analysis::ac;
    std::stringstream ss{std::string(line)};
    std::string opcond, ac_analysis_mode, num_of_point, f_start, f_end;

    ss >> opcond >> ac_analysis_mode >> num_of_point >> f_start >> f_end;

    // Determine sweep mode
    if (ac_analysis_mode == "oct" || ac_analysis_mode == "OCT") {
        netlist->ac_analysis = AcAnalysis::oct;
    } else if (ac_analysis_mode == "dec" || ac_analysis_mode == "DEC") {
        netlist->ac_analysis = AcAnalysis::dec;
    } else if (ac_analysis_mode == "lin" || ac_analysis_mode == "LIN") {
        netlist->ac_analysis = AcAnalysis::lin;
    } else if (ac_analysis_mode == "list" || ac_analysis_mode == "LIST") {
        netlist->ac_analysis = AcAnalysis::list;
    }

    netlist->n       = parseSpiceValue(num_of_point);
    netlist->f_start = parseSpiceValue(f_start);
    netlist->f_end   = parseSpiceValue(f_end);
}

/**
 * @brief Parses independent voltage sources (DC and AC).
 *
 * Reads source operational parameters (amplitude, phase, DC offset) based on
 * mode descriptors and adds the source to the netlist.
 *
 * @param ss Pointer to stringstream reading the source line parameters.
 * @param val_str Value string or mode descriptor (e.g., "DC", "AC").
 * @param positive_node Name of the positive node terminal.
 * @param negative_node Name of the negative node terminal.
 * @param name Unique identifier of the voltage source.
 */
void Parser::parseVoltageSource(
    std::stringstream* ss,
    std::string val_str,
    std::string positive_node,
    std::string negative_node,
    std::string name
) {
    if (val_str == "AC" || val_str == "ac") {
        std::string amplitude, phase;
        (*ss) >> amplitude >> phase;

        IndependentVoltageSource voltage_source(
            name,
            netlist->node_name_to_index[positive_node],
            netlist->node_name_to_index[negative_node],
            SourceMode::AC
        );

        voltage_source.amplitude = parseSpiceValue(amplitude);
        voltage_source.phase     = parseSpiceValue(phase);

        netlist->voltage_sources.push_back(voltage_source);

    } else if (val_str == "DC" || val_str == "dc") {
        std::string dc_offset;
        (*ss) >> dc_offset;

        IndependentVoltageSource voltage_source(
            name,
            netlist->node_name_to_index[positive_node],
            netlist->node_name_to_index[negative_node],
            SourceMode::DC
        );

        voltage_source.dc_offset = parseSpiceValue(dc_offset);

        netlist->voltage_sources.push_back(voltage_source);

    } else {
        // Default case: Value given without explicit "DC" prefix
        IndependentVoltageSource voltage_source(
            name,
            netlist->node_name_to_index[positive_node],
            netlist->node_name_to_index[negative_node],
            SourceMode::DC
        );

        voltage_source.dc_offset = parseSpiceValue(val_str);

        netlist->voltage_sources.push_back(voltage_source);
    }
}