/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : mna_system_tran.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the MnaSystemTran class, which acts as the container for
 *  the real-valued dense Modified Nodal Analysis (MNA) differential matrix system
 *  (C * dx/dt + G * x = b) used in Transient (.tran) time-domain simulations.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *    MnaBuilderTran
 *            ↓
 *     MnaSystemTran     <-- [This File]
 *            ↓
 *    SimulationRunner
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <cstdint>

#include <eigen3/Eigen/Dense>

#include "netlist.hpp"

/**
 * @brief Holds and manages the real-valued MNA matrices G, C, and RHS vector b for transient simulations.
 */
class MnaSystemTran {
private:
    /**
     * Pointer to the associated circuit netlist structure.
     */
    Netlist* netlist{nullptr};

public:
    /**
     * System conductance matrix G (size N x N).
     */
    Eigen::MatrixXd G;

    /**
     * System dynamic capacitance/inductance matrix C (size N x N).
     */
    Eigen::MatrixXd C;

    /**
     * Right-hand side excitation vector b (size N x 1).
     */
    Eigen::VectorXd b;

    /**
     * @brief Constructs and initializes transient MNA matrices and vectors to zero.
     *
     * @param netlist Pointer to the parsed netlist object.
     */
    explicit MnaSystemTran(Netlist* netlist) : netlist(netlist) {
        if (netlist != nullptr) {
            const int32_t order_mna = netlist->num_nodes + netlist->num_voltage_sources + netlist->num_inductors;

            // Initialize Eigen matrices and vectors with zeros
            G = Eigen::MatrixXd::Zero(order_mna, order_mna);
            C = Eigen::MatrixXd::Zero(order_mna, order_mna);
            b = Eigen::VectorXd::Zero(order_mna);
        }
    }

    /**
     * @brief Default destructor.
     */
    ~MnaSystemTran() noexcept = default;
};