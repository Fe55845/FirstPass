/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : mna_builder_ac.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the MnaBuilderAc class, which builds and updates the
 *  complex-valued Modified Nodal Analysis (MNA) system matrices (G and b)
 *  for AC frequency-domain simulations at a specific angular frequency (omega).
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *     MnaBuilderAc      <-- [This File]
 *            ↓
 *       MnaSystemAc
 *            ↓
 *    SimulationRunner
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *  In AC small-signal analysis, elements are represented by their complex admittance:
 *    - Resistors:   Y = 1 / R
 *    - Capacitors:  Y = j * ω * C
 *    - Inductors:   Y = 1 / (j * ω * L) = -j / (ω * L)
 *
 *  Nodes use signed 32-bit integers, where GROUND_NODE = -1 represents reference ground ("0").
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <cmath>
#include <complex>
#include <cstdint>
#include <string>

#include <eigen3/Eigen/Sparse>

#include "mna_system_ac.hpp"
#include "netlist.hpp"
#include "simulation_results.hpp"

/**
 * @brief Constructs and updates complex MNA matrices for AC simulation frequency sweeps.
 */
class MnaBuilderAc {
private:
    /**
     * Pointer to the target complex MNA equation container.
     */
    MnaSystemAc* mna_equation{nullptr};

    /**
     * Pointer to the parsed netlist topology data.
     */
    Netlist* netlist{nullptr};

    /**
     * @brief Stamps a resistor conductance into the complex MNA system.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param value Resistance value in Ohms.
     */
    void add_resistor(int32_t positive_node, int32_t negative_node, double value) {
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
     * @brief Stamps a capacitor admittance into the complex MNA system.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param value Capacitance value in Farads.
     * @param omega Angular frequency (rad/s).
     */
    void add_capacitor(int32_t positive_node, int32_t negative_node, double value, double omega) {
        const std::complex<double> y(0.0, value * omega);

        if (positive_node == netlist->GROUND_NODE) {
            mna_equation->addG(negative_node, negative_node, y);
        } else if (negative_node == netlist->GROUND_NODE) {
            mna_equation->addG(positive_node, positive_node, y);
        } else {
            mna_equation->addG(positive_node, positive_node, y);
            mna_equation->addG(positive_node, negative_node, -y);
            mna_equation->addG(negative_node, positive_node, -y);
            mna_equation->addG(negative_node, negative_node, y);
        }
    }

    /**
     * @brief Stamps an inductor admittance into the complex MNA system.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param value Inductance value in Henries.
     * @param omega Angular frequency (rad/s).
     */
    void add_inductor(int32_t positive_node, int32_t negative_node, double value, double omega) {
        const std::complex<double> y(0.0, -1.0 / (value * omega));

        if (positive_node == netlist->GROUND_NODE) {
            mna_equation->addG(negative_node, negative_node, y);
        } else if (negative_node == netlist->GROUND_NODE) {
            mna_equation->addG(positive_node, positive_node, y);
        } else {
            mna_equation->addG(positive_node, positive_node, y);
            mna_equation->addG(positive_node, negative_node, -y);
            mna_equation->addG(negative_node, positive_node, -y);
            mna_equation->addG(negative_node, negative_node, y);
        }
    }

    /**
     * @brief Stamps an independent current source into the RHS vector b.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param value Source current magnitude.
     */
    void add_current_source(int32_t positive_node, int32_t negative_node, double value) {
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
     * @brief Stamps an independent AC voltage source into G and RHS vector b.
     *
     * @param positive_node Signed index of the positive terminal node.
     * @param negative_node Signed index of the negative terminal node.
     * @param aux_index Auxiliary variable index offset for source current.
     * @param value Phasor representation of source voltage.
     */
    void add_voltage_source(
        int32_t positive_node,
        int32_t negative_node,
        int32_t aux_index,
        std::complex<double> value
    ) {
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
     * @brief Constructs an MnaBuilderAc instance.
     *
     * @param mna_equation Pointer to the target complex MNA equation structure.
     * @param netlist Pointer to the parsed netlist topology structure.
     */
    MnaBuilderAc(MnaSystemAc* mna_equation, Netlist* netlist)
        : mna_equation(mna_equation), netlist(netlist) {}

    /**
     * @brief Re-stamps and updates the system matrix G and vector b for a given frequency.
     *
     * @param omega Angular frequency in rad/s (ω = 2 * π * f).
     */
    void update_mna(double omega) {
        mna_equation->G.setZero();
        mna_equation->b.setZero();

        int32_t num_of_sources = 0;
        int32_t aux_index       = 0;

        // Iterate through passive components and stamp admittance contributions
        for (const auto& comp : netlist->passive_components) {
            switch (comp.name[0]) {
                case 'R': case 'r':
                    add_resistor(comp.positive_node, comp.negative_node, comp.value);
                    break;
                case 'C': case 'c':
                    add_capacitor(comp.positive_node, comp.negative_node, comp.value, omega);
                    break;
                case 'L': case 'l':
                    add_inductor(comp.positive_node, comp.negative_node, comp.value, omega);
                    break;
                case 'I': case 'i':
                    add_current_source(comp.positive_node, comp.negative_node, comp.value);
                    num_of_sources++;
                    break;
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

            // Construct AC phasor value: A * e^(j*phi)
            const std::complex<double> voltage_phasor = std::polar(src.amplitude, src.phase);

            add_voltage_source(
                src.positive_node,
                src.negative_node,
                aux_index,
                voltage_phasor
            );
            aux_index++;
        }

        mna_equation->finalizeMatrix();
    }

    /**
     * @brief Default destructor.
     */
    ~MnaBuilderAc() = default;
};