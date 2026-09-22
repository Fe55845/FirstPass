/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : mna_builder_op.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the MnaBuilderOp class, which constructs the real-valued
 *  Modified Nodal Analysis (MNA) system matrices (G and b) for DC Operating
 *  Point (.op) calculations.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *     MnaBuilderOp      <-- [This File]
 *            ↓
 *       MnaSystemOp
 *            ↓
 *    SimulationRunner
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *  In DC Operating Point analysis:
 *    - Capacitors behave as Open Circuits (ignored).
 *    - Inductors behave as Short Circuits (stamped as 0V voltage sources).
 *    - Resistors are stamped into the G matrix as G_elem = 1 / R.
 *    - Independent voltage sources use their DC offset value.
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

#include <eigen3/Eigen/Sparse>

#include "mna_system_op.hpp"
#include "netlist.hpp"

/**
 * @brief Constructs real MNA matrices for DC operating point analysis.
 */
class MnaBuilderOp {
private:
    /**
     * Pointer to the target real MNA equation container.
     */
    MnaSystemOp* mna_equation;

    /**
     * Pointer to the parsed netlist topology data.
     */
    Netlist* netlist;

    /**
     * @brief Stamps a resistor conductance into the DC MNA system.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param value Resistance value in Ohms.
     */
    void add_resistor(int32_t positive_node, int32_t negative_node, double value) const {
        const double g = 1.0 / value;

        if (positive_node == netlist->GROUND_NODE) {
            mna_equation->addG(negative_node, negative_node, g);
        } else if (negative_node == netlist->GROUND_NODE) {
            mna_equation->addG(positive_node, positive_node, g);
        } else {
            mna_equation->addG(positive_node, positive_node, g);
            mna_equation->addG(positive_node, negative_node, -g);
            mna_equation->addG(negative_node, positive_node, -g);
            mna_equation->addG(negative_node, negative_node, g);
        }
    }

    /**
     * @brief Stamps an independent DC current source into RHS vector b.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param value Source current magnitude.
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
     * @brief Stamps an independent DC voltage source into G and RHS vector b.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param aux_index Auxiliary variable index offset for source current.
     * @param value Source DC voltage value.
     */
    void add_voltage_source(
        int32_t positive_node,
        int32_t negative_node,
        int32_t aux_index,
        double value
    ) const {
        const int32_t source_row = netlist->num_nodes + aux_index;

        if (positive_node == netlist->GROUND_NODE) {
            mna_equation->addG(negative_node, source_row, -1.0);
            mna_equation->addG(source_row, negative_node, -1.0);
            (mna_equation->b)(source_row) += value;
        } else if (negative_node == netlist->GROUND_NODE) {
            mna_equation->addG(positive_node, source_row, 1.0);
            mna_equation->addG(source_row, positive_node, 1.0);
            (mna_equation->b)(source_row) += value;
        } else {
            mna_equation->addG(positive_node, source_row, 1.0);
            mna_equation->addG(source_row, positive_node, 1.0);
            mna_equation->addG(negative_node, source_row, -1.0);
            mna_equation->addG(source_row, negative_node, -1.0);
            (mna_equation->b)(source_row) += value;
        }
    }

public:
    /**
     * @brief Constructs the MNA system by iterating through all netlist components.
     *
     * @param mna_equation Pointer to the target real MNA equation structure.
     * @param netlist Pointer to the parsed netlist structure.
     */
    explicit MnaBuilderOp(MnaSystemOp* mna_equation, Netlist* netlist)
        : mna_equation(mna_equation), netlist(netlist) {

        int32_t aux_index = 0;

        // Iterate through passive components and stamp contributions
        for (const auto& comp : netlist->passive_components) {
            switch (comp.name[0]) {
                case 'R': case 'r':
                    add_resistor(comp.positive_node, comp.negative_node, comp.value);
                    break;

                // Capacitors are ignored in DC operating point analysis (open circuit)

                // Inductors act as short circuits (stamped as 0V voltage sources)
                case 'L': case 'l': {
                    const std::string aux_name = "i_" + comp.name;
                    const int32_t aux_row      = netlist->num_nodes + aux_index;

                    netlist->node_name_to_index[aux_name] = aux_row;
                    netlist->index_to_node_name[aux_row]  = aux_name;

                    add_voltage_source(comp.positive_node, comp.negative_node, aux_index, 0.0);
                    aux_index++;
                    break;
                }

                case 'I': case 'i':
                    add_current_source(comp.positive_node, comp.negative_node, comp.value);
                    break;

                default:
                    break;
            }
        }

        // Iterate through independent voltage sources and stamp equations
        for (const auto& src : netlist->voltage_sources) {
            const std::string aux_name = "i_" + src.name;
            const int32_t aux_row      = netlist->num_nodes + aux_index;

            netlist->node_name_to_index[aux_name] = aux_row;
            netlist->index_to_node_name[aux_row]  = aux_name;

            add_voltage_source(src.positive_node, src.negative_node, aux_index, src.dc_offset);
            aux_index++;
        }

        mna_equation->finalizeMatrix();
    }

    /**
     * @brief Default destructor.
     */
    ~MnaBuilderOp() noexcept = default;
};