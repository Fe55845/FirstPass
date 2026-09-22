/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : mna_builder_tran.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the MnaBuilderTran class, which builds the time-domain
 *  Modified Nodal Analysis (MNA) system matrices (G, C, and b) for Transient
 *  (.tran) simulations.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *    MnaBuilderTran     <-- [This File]
 *            ↓
 *      MnaSystemTran
 *            ↓
 *    SimulationRunner
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *  In transient analysis, the differential MNA system is expressed as:
 *      C * dx/dt + G * x = b(t)
 *
 *    - Conductances (Resistors) are stamped into matrix G.
 *    - Capacitances are stamped into matrix C.
 *    - Inductors are stamped with an auxiliary current equation into both G and C.
 *    - Independent voltage and current sources update RHS vector b.
 *
 *  Nodes use signed 32-bit integers, where GROUND_NODE = -1 represents reference ground ("0").
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <cstdint>
#include <string>

#include <eigen3/Eigen/Dense>

#include "mna_system_tran.hpp"
#include "netlist.hpp"

/**
 * @brief Constructs MNA matrices G, C, and vector b for transient time-domain simulations.
 */
class MnaBuilderTran {
private:
    /**
     * Pointer to the target transient MNA equation container.
     */
    MnaSystemTran* mna_equation{nullptr};

    /**
     * Pointer to the parsed netlist topology data.
     */
    Netlist* netlist{nullptr};

    /**
     * @brief Stamps a resistor conductance into matrix G.
     */
    void add_resistor(int32_t positive_node, int32_t negative_node, double value) const {
        const double g = 1.0 / value;

        if (positive_node == netlist->GROUND_NODE) {
            (mna_equation->G)(negative_node, negative_node) += g;
        } else if (negative_node == netlist->GROUND_NODE) {
            (mna_equation->G)(positive_node, positive_node) += g;
        } else {
            (mna_equation->G)(positive_node, positive_node) += g;
            (mna_equation->G)(positive_node, negative_node) -= g;
            (mna_equation->G)(negative_node, positive_node) -= g;
            (mna_equation->G)(negative_node, negative_node) += g;
        }
    }

    /**
     * @brief Stamps a capacitor value into matrix C.
     */
    void add_capacitor(int32_t positive_node, int32_t negative_node, double value) const {
        if (positive_node == netlist->GROUND_NODE) {
            (mna_equation->C)(negative_node, negative_node) += value;
        } else if (negative_node == netlist->GROUND_NODE) {
            (mna_equation->C)(positive_node, positive_node) += value;
        } else {
            (mna_equation->C)(positive_node, positive_node) += value;
            (mna_equation->C)(positive_node, negative_node) -= value;
            (mna_equation->C)(negative_node, positive_node) -= value;
            (mna_equation->C)(negative_node, negative_node) += value;
        }
    }

    /**
     * @brief Stamps an inductor into matrices G and C with an extra branch current variable.
     */
    void add_inductor(
        int32_t positive_node,
        int32_t negative_node,
        int32_t aux_index,
        double value
    ) const {
        const int32_t source_row = netlist->num_nodes + aux_index;

        if (positive_node == netlist->GROUND_NODE) {
            (mna_equation->G)(negative_node, source_row) -= 1.0;
            (mna_equation->G)(source_row, negative_node) -= 1.0;
            (mna_equation->C)(source_row, source_row) -= value;
        } else if (negative_node == netlist->GROUND_NODE) {
            (mna_equation->G)(positive_node, source_row) += 1.0;
            (mna_equation->G)(source_row, positive_node) += 1.0;
            (mna_equation->C)(source_row, source_row) -= value;
        } else {
            (mna_equation->G)(negative_node, source_row) -= 1.0;
            (mna_equation->G)(positive_node, source_row) += 1.0;
            (mna_equation->G)(source_row, negative_node) -= 1.0;
            (mna_equation->G)(source_row, positive_node) += 1.0;
            (mna_equation->C)(source_row, source_row) -= value;
        }
    }

    /**
     * @brief Stamps an independent current source into RHS vector b.
     */
    void add_current_source(int32_t positive_node, int32_t negative_node, double value) const {
        if (positive_node == netlist->GROUND_NODE) {
            (mna_equation->b)(negative_node) += value;
        } else if (negative_node == netlist->GROUND_NODE) {
            (mna_equation->b)(positive_node) -= value;
        } else {
            (mna_equation->b)(negative_node) += value;
            (mna_equation->b)(positive_node) -= value;
        }
    }

    /**
     * @brief Stamps an independent voltage source into matrix G and RHS vector b.
     */
    void add_voltage_source(
        int32_t positive_node,
        int32_t negative_node,
        int32_t aux_index,
        double value
    ) const {
        const int32_t source_row = netlist->num_nodes + aux_index;

        if (positive_node == netlist->GROUND_NODE) {
            (mna_equation->G)(negative_node, source_row) -= 1.0;
            (mna_equation->G)(source_row, negative_node) -= 1.0;
            (mna_equation->b)(source_row) += value;
        } else if (negative_node == netlist->GROUND_NODE) {
            (mna_equation->G)(positive_node, source_row) += 1.0;
            (mna_equation->G)(source_row, positive_node) += 1.0;
            (mna_equation->b)(source_row) += value;
        } else {
            (mna_equation->G)(positive_node, source_row) += 1.0;
            (mna_equation->G)(source_row, positive_node) += 1.0;
            (mna_equation->G)(negative_node, source_row) -= 1.0;
            (mna_equation->G)(source_row, negative_node) -= 1.0;
            (mna_equation->b)(source_row) += value;
        }
    }

public:
    /**
     * @brief Constructs the transient MNA system matrices by iterating through netlist components.
     *
     * @param mna_equation Pointer to the target transient MNA equation structure.
     * @param netlist Pointer to the parsed netlist structure.
     */
    explicit MnaBuilderTran(MnaSystemTran* mna_equation, Netlist* netlist)
        : mna_equation(mna_equation), netlist(netlist) {

        int32_t aux_index = 0;

        // Iterate through passive components and stamp matrices G, C, and b
        for (const auto& comp : netlist->passive_components) {
            switch (comp.name[0]) {
                case 'R': case 'r':
                    add_resistor(comp.positive_node, comp.negative_node, comp.value);
                    break;
                case 'I': case 'i':
                    add_current_source(comp.positive_node, comp.negative_node, comp.value);
                    break;
                case 'C': case 'c':
                    add_capacitor(comp.positive_node, comp.negative_node, comp.value);
                    break;
                case 'L': case 'l': {
                    const std::string aux_name = "i_" + comp.name;
                    const int32_t aux_row      = netlist->num_nodes + aux_index;

                    netlist->node_name_to_index[aux_name] = aux_row;
                    netlist->index_to_node_name[aux_row]  = aux_name;

                    add_inductor(comp.positive_node, comp.negative_node, aux_index, comp.value);
                    aux_index++;
                    break;
                }
                default:
                    break;
            }
        }

        // Iterate through voltage sources and stamp constraint equations
        for (const auto& src : netlist->voltage_sources) {
            const std::string aux_name = "i_" + src.name;
            const int32_t aux_row      = netlist->num_nodes + aux_index;

            netlist->node_name_to_index[aux_name] = aux_row;
            netlist->index_to_node_name[aux_row]  = aux_name;

            add_voltage_source(src.positive_node, src.negative_node, aux_index, src.dc_offset);
            aux_index++;
        }
    }

    /**
     * @brief Default destructor.
     */
    ~MnaBuilderTran() noexcept = default;
};