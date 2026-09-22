/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : mna_system_ac.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the MnaSystemAc class, which acts as the container for
 *  the complex-valued Modified Nodal Analysis (MNA) sparse matrix system
 *  (G * x = b) used in AC frequency-domain simulations.
 *
 *  Application Workflow
 *  --------------------------------------------------------------------------
 *      SPICE Netlist
 *            ↓
 *         Parser
 *            ↓
 *        Netlist
 *            ↓
 *     MnaBuilderAc
 *            ↓
 *      MnaSystemAc      <-- [This File]
 *            ↓
 *    SimulationRunner
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <complex>
#include <cstdint>
#include <vector>

#include <eigen3/Eigen/Sparse>

#include "netlist.hpp"

/**
 * @brief Holds and manages the complex MNA system matrices G, RHS vector b, and solution vector x.
 */
class MnaSystemAc {
private:
    /**
     * Pointer to the associated circuit netlist structure.
     */
    Netlist* netlist{nullptr};

    /**
     * Temporary triplet list used for efficient sparse matrix assembly via setFromTriplets.
     */
    std::vector<Eigen::Triplet<std::complex<double>>> triplets;

public:
    /**
     * System conductance/admittance sparse matrix G (size N x N).
     */
    Eigen::SparseMatrix<std::complex<double>> G;

    /**
     * Right-hand side excitation vector b (size N x 1).
     */
    Eigen::VectorXcd b;

    /**
     * Solution vector x containing complex node voltages and branch currents (size N x 1).
     */
    Eigen::VectorXcd x;

    /**
     * @brief Constructs and initializes MNA matrices and vectors.
     *
     * @param netlist Pointer to the parsed netlist object.
     */
    explicit MnaSystemAc(Netlist* netlist) : netlist(netlist) {
        if (netlist != nullptr) {
            const int32_t n = netlist->order_mna_equation;

            G = Eigen::SparseMatrix<std::complex<double>>(n, n);
            b = Eigen::VectorXcd::Zero(n);
            x = Eigen::VectorXcd::Zero(n);

            // Reserve memory for triplets to prevent frequent re-allocations
            // Typical MNA matrices have ~3 to 5 non-zero entries per row/column
            triplets.reserve(static_cast<std::size_t>(n) * 4);
        }
    }

    /**
     * @brief Adds a component admittance stamp to the temporary triplet list.
     *
     * @param r 0-based row index.
     * @param c 0-based column index.
     * @param val Complex admittance/stamp value to add.
     */
    void addG(int32_t r, int32_t c, std::complex<double> val) {
        if (val != 0.0) {
            triplets.emplace_back(r, c, val);
        }
    }

    /**
     * @brief Builds the final compressed sparse matrix G from accumulated triplets.
     *
     * Automatically sums up overlapping entries (e.g. parallel admittances at same node)
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
    ~MnaSystemAc() noexcept = default;
};