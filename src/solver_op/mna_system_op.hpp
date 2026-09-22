/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : mna_system_op.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the MnaSystemOp class, which acts as the container for
 *  the real-valued Modified Nodal Analysis (MNA) sparse matrix system
 *  (G * x = b) used in DC Operating Point (.op) simulations.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *     MnaBuilderOp
 *            ↓
 *      MnaSystemOp      <-- [This File]
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
#include <vector>

#include <eigen3/Eigen/Sparse>

#include "netlist.hpp"

/**
 * @brief Holds and manages the real MNA system matrices G, RHS vector b, and solution vector x.
 */
class MnaSystemOp {
private:
    /**
     * Pointer to the associated circuit netlist structure.
     */
    Netlist* netlist{nullptr};

    /**
     * Temporary triplet list used for efficient sparse matrix assembly via setFromTriplets.
     */
    std::vector<Eigen::Triplet<double>> triplets;

public:
    /**
     * System conductance sparse matrix G (size N x N).
     */
    Eigen::SparseMatrix<double> G;

    /**
     * Right-hand side excitation vector b (size N x 1).
     */
    Eigen::VectorXd b;

    /**
     * Solution vector x containing node voltages and branch currents (size N x 1).
     */
    Eigen::VectorXd x;

    /**
     * @brief Constructs and initializes MNA matrices and vectors to zero.
     *
     * @param netlist Pointer to the parsed netlist object.
     */
    explicit MnaSystemOp(Netlist* netlist) : netlist(netlist) {
        if (netlist != nullptr) {
            const int32_t n = netlist->order_mna_equation;

            G = Eigen::SparseMatrix<double>(n, n);
            b = Eigen::VectorXd::Zero(n);
            x = Eigen::VectorXd::Zero(n);

            // Reserve memory for triplets to prevent frequent re-allocations
            triplets.reserve(static_cast<std::size_t>(n) * 4);
        }
    }

    /**
     * @brief Adds a component conductance stamp to the temporary triplet list.
     *
     * @param r 0-based row index.
     * @param c 0-based column index.
     * @param val Conductance/stamp value to add.
     */
    void addG(int32_t r, int32_t c, double val) {
        if (val != 0.0) {
            triplets.emplace_back(r, c, val);
        }
    }

    /**
     * @brief Builds the final compressed sparse matrix G from accumulated triplets.
     *
     * Automatically sums up overlapping entries (e.g. parallel conductances at same node)
     * and compresses matrix memory representation for efficient linear solving.
     */
    void finalizeMatrix() {
        G.setFromTriplets(triplets.begin(), triplets.end());
        G.makeCompressed();

        // Release temporary triplet memory
        triplets.clear();
        triplets.shrink_to_fit();
    }

    /**
     * @brief Default destructor.
     */
    ~MnaSystemOp() noexcept = default;
};