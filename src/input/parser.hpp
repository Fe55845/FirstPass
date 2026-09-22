/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : parser.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the Parser class, which is responsible for parsing SPICE
 *  netlist files and converting textual circuit descriptions and simulation
 *  directives into internal data structures.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser        <-- [This File]
 *            ↓
 *        Netlist
 *            ↓
 *    SimulationRunner
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *  The parser converts string-based component declarations and SPICE engineering
 *  prefixes (e.g., 'k', 'u', 'm', 'Meg') into numerical floating-point values
 *  for numerical matrix generation.
 *
 *  Nodes are mapped to consecutive zero-based integer indices, with the ground
 *  reference node ("0") defined as index -1.
 *
 *  Design Notes
 *  --------------------------------------------------------------------------
 *  To optimize parsing performance and avoid excessive memory allocations during
 *  string operations, non-owning std::string_view primitives and fast string-to-number
 *  conversions via std::from_chars are utilized wherever possible.
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <charconv>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

#include "netlist.hpp"

/**
 * @brief Parses SPICE netlists and fills the central Netlist data structure.
 *
 * The Parser processes SPICE input line-by-line, identifies component values,
 * maps node names to integer indices, and extracts simulation directives
 * such as .op, .ac, and .tran.
 */
class Parser {
private:
    /**
     * Pointer to the target Netlist structure.
     */
    Netlist* netlist{nullptr};

    /**
     * @brief High-performance SPICE numerical value parser.
     *
     * Converts a string representation of a numerical value including SPICE
     * scale prefixes (e.g., "1k", "10u", "2.2Meg") into a double-precision float.
     *
     * @param sv String view containing the numerical value and suffix.
     * @return Converted double-precision floating point value.
     */
    double parseSpiceValue(std::string_view sv);

    /**
     * @brief Parses a transient simulation directive (.tran).
     *
     * Extracts simulation timing parameters such as start time, stop time, and sample step.
     *
     * @param line Raw string view containing the directive line.
     */
    void parseTransientDirective(std::string_view line);

    /**
     * @brief Parses an AC analysis directive (.ac).
     *
     * Extracts sweep parameters such as sweep mode, number of points, and frequency limits.
     *
     * @param line Raw string view containing the directive line.
     */
    void parseAcDirective(std::string_view line);

    /**
     * @brief Parses independent voltage source definitions.
     *
     * Handles DC and AC voltage source parameters and populates source definitions.
     *
     * @param ss Pointer to stringstream reading the source line parameters.
     * @param val_str Value string or mode descriptor (e.g. "DC", "AC").
     * @param positive_node Name of the positive node terminal.
     * @param negative_node Name of the negative node terminal.
     * @param name Unique identifier of the voltage source.
     */
    void parseVoltageSource(
        std::stringstream* ss,
        std::string val_str,
        std::string positive_node,
        std::string negative_node,
        std::string name
    );

public:
    /**
     * @brief Constructs a Parser instance and directly processes the specified SPICE netlist file.
     *
     * Parses all circuit elements, assigns node indices, counts components, and identifies
     * configured analysis directives.
     *
     * @param file_name Path to the SPICE input file.
     * @param netlist Pointer to the Netlist structure where parsed data will be stored.
     */
    Parser(const std::string& file_name, Netlist* netlist) : netlist(netlist) {
        std::ifstream infile(file_name, std::ios::in | std::ios::binary | std::ios::ate);
        if (!infile.is_open()) {
            return;
        }

        std::streamsize size = infile.tellg();
        infile.seekg(0, std::ios::beg);

        std::string file_content;
        file_content.resize(size);

        // Initialize ground node mapping
        netlist->node_name_to_index["0"] = netlist->GROUND_NODE;
        netlist->index_to_node_name[netlist->GROUND_NODE]  = "0";

        // Reset component counters
        netlist->num_resistors       = 0;
        netlist->num_inductors       = 0;
        netlist->num_capacitors      = 0;
        netlist->num_current_sources = 0;
        netlist->num_voltage_sources = 0;

        netlist->passive_components.reserve(size / 40);

        uint32_t index_num_of_nodes = 0;

        if (infile.read(file_content.data(), size)) {
            std::string_view content_view(file_content);
            size_t start = 0;

            while (start < content_view.size()) {
                size_t end = content_view.find('\n', start);
                std::string_view line = content_view.substr(start, end - start);

                // Prepare next line offset
                start = (end == std::string_view::npos) ? content_view.size() : end + 1;

                std::string_view sv(line);

                // Trim leading whitespace
                size_t first = sv.find_first_not_of(" \t\r\n");
                if (first == std::string_view::npos) {
                    continue; // Skip empty line
                }
                sv.remove_prefix(first);

                // Skip SPICE comments ('*' and ';')
                if (sv[0] == '*' || sv[0] == ';') {
                    continue;
                }

                // Process simulator directives
                if (sv[0] == '.') {
                    if (sv.rfind(".tran", 0) == 0) {
                        parseTransientDirective(line);
                    } else if (sv.rfind(".ac", 0) == 0) {
                        parseAcDirective(line);
                    } else if (sv.rfind(".dc", 0) == 0) {
                        netlist->analysis = Analysis::dc;
                    } else if (sv.rfind(".op", 0) == 0) {
                        netlist->analysis = Analysis::op;
                    }
                    continue;
                }

                // Parse component definition: name node1 node2 value
                std::stringstream ss{std::string(line)};
                std::string name, positive_node, negative_node, val_str;

                if (ss >> name >> positive_node >> negative_node >> val_str) {
                    switch (name[0]) {
                        case 'R': case 'r': netlist->num_resistors++; break;
                        case 'L': case 'l': netlist->num_inductors++; break;
                        case 'C': case 'c': netlist->num_capacitors++; break;
                        case 'I': case 'i': netlist->num_current_sources++; break;
                        case 'V': case 'v': netlist->num_voltage_sources++; break;
                        default: break;
                    }

                    if (netlist->node_name_to_index.find(positive_node) == netlist->node_name_to_index.end()) {
                        netlist->node_name_to_index[positive_node] = index_num_of_nodes;
                        netlist->index_to_node_name[index_num_of_nodes] = positive_node;
                        index_num_of_nodes++;
                    }

                    if (netlist->node_name_to_index.find(negative_node) == netlist->node_name_to_index.end()) {
                        netlist->node_name_to_index[negative_node] = index_num_of_nodes;
                        netlist->index_to_node_name[index_num_of_nodes] = negative_node;
                        index_num_of_nodes++;
                    }

                    if (name[0] == 'V' || name[0] == 'v') {
                        parseVoltageSource(&ss, val_str, positive_node, negative_node, name);
                    } else {
                        double value = parseSpiceValue(val_str);

                        netlist->passive_components.push_back(PassiveComponent(
                            name,
                            netlist->node_name_to_index[positive_node],
                            netlist->node_name_to_index[negative_node],
                            value
                        ));
                    }
                }
            }
        }

        netlist->num_nodes = static_cast<uint32_t>(netlist->node_name_to_index.size() - 1);
    }

    /**
     * @brief Default destructor.
     */
    ~Parser() = default;
};