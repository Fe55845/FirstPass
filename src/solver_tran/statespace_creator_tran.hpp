/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : statespace_creator_tran.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the StateSpaceCreator class, which extracts a reduced-order
 *  state-space representation (F, g, X_z, X_b) from the singular MNA differential
 *  system (C * dx/dt + G * x = b) using Singular Value Decomposition (SVD).
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
 *     MnaSystemTran
 *            ↓
 *  StateSpaceCreator    <-- [This File]
 *            ↓
 *   StateSpaceModel
 *            ↓
 *       SolverTran
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *  1. SVD of Capacitance/Inductance Matrix C:
 *         C = U * S * V^T
 *  2. Partitioning into dynamic subspace (rank n) and algebraic subspace (rank p = N - n).
 *  3. Inversion of algebraic coupling block (U2^T * G * V2).
 *  4. Derivation of minimal differential state system:
 *         dz1/dt = F * z1 + g
 *         x      = X_z * z1 + X_b * b
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <cstdint>
#include <limits>
#include <string>

#include <eigen3/Eigen/Dense>

#include "mna_system_tran.hpp"
#include "netlist.hpp"
#include "simulation_results.hpp"
#include "statespace_model_tran.hpp"

/**
 * @brief Derives state-space representation matrices from MNA differential equations via SVD.
 */
class StateSpaceCreator {
private:
    /**
     * Pointer to the target state-space model container.
     */
    StateSpaceModel* state_space{nullptr};

    /**
     * Pointer to the source transient MNA system matrices (G, C, b).
     */
    MnaSystemTran* mna_equation{nullptr};

    /**
     * Pointer to the current circuit netlist topology and setup object.
     */
    Netlist* netlist{nullptr};

public:
    /**
     * @brief Constructs state-space matrices (F, g, X_z, X_b) via SVD reduction of matrix C.
     *
     * @param netlist Pointer to the circuit netlist configuration.
     * @param state_space Pointer to the target state-space model container.
     * @param mna_equation Pointer to the source transient MNA system.
     */
    explicit StateSpaceCreator(
        Netlist* netlist,
        StateSpaceModel* state_space,
        MnaSystemTran* mna_equation
    ) : state_space(state_space),
        mna_equation(mna_equation),
        netlist(netlist) {

        // 1. Compute SVD of C matrix: C = U * S * V^T
        // Full U and V matrices are required to extract null space bases U2 and V2
        Eigen::BDCSVD<Eigen::MatrixXd, Eigen::ComputeFullU | Eigen::ComputeFullV> svd(mna_equation->C);

        svd.setThreshold(std::numeric_limits<double>::epsilon());

        const Eigen::VectorXd singularValues = svd.singularValues();
        const Eigen::MatrixXd U = svd.matrixU();
        const Eigen::MatrixXd V = svd.matrixV();

        // 2. Determine state-space rank 'n' and algebraic dimension 'p'
        const int32_t n = static_cast<int32_t>(svd.rank());
        netlist->order_state_space = n;

        const int32_t N = static_cast<int32_t>(mna_equation->C.rows());
        const int32_t p = N - n;

        // 3. Partition matrices into dynamic (n) and algebraic (p) subspaces
        const Eigen::MatrixXd U1 = U.leftCols(n);              // (N x n)
        const Eigen::MatrixXd V1 = V.leftCols(n);              // (N x n)
        const Eigen::VectorXd S1_vec = singularValues.head(n); // (n)

        const Eigen::MatrixXd U2 = U.rightCols(p); // (N x p)
        const Eigen::MatrixXd V2 = V.rightCols(p); // (N x p)

        // Inverse of non-zero singular value diagonal matrix S1
        const Eigen::MatrixXd Sigma1_inv = S1_vec.cwiseInverse().asDiagonal(); // (n x n)

        // 4. Invert algebraic coupling block: (U2^T * G * V2)
        const Eigen::MatrixXd A_tilde_22 = U2.transpose() * mna_equation->G * V2;
        const Eigen::PartialPivLU<Eigen::MatrixXd> lu_alg(A_tilde_22);

        // Compute auxiliary transformation matrices for algebraic variables
        const Eigen::MatrixXd W_from_Z = -lu_alg.solve(U2.transpose() * mna_equation->G * V1); // (p x n)
        const Eigen::MatrixXd W_from_B =  lu_alg.solve(U2.transpose());                         // (p x N)

        // 5. Build dynamic state-transition matrix F and input vector g:
        // dz1/dt = F * z1 + g
        const Eigen::MatrixXd A_tilde_reduced = U1.transpose() * mna_equation->G * (V1 + V2 * W_from_Z); // (n x n)
        state_space->F = -Sigma1_inv * A_tilde_reduced;

        const Eigen::MatrixXd B_sys = Sigma1_inv * (U1.transpose() - U1.transpose() * mna_equation->G * V2 * W_from_B);
        state_space->g = B_sys * mna_equation->b;

        // 6. Build output mapping matrices to recover original MNA vector x:
        // x = V1 * z1 + V2 * z2  =>  x = X_z * z1 + X_b * b
        state_space->X_from_z = V1 + V2 * W_from_Z; // (N x n)
        state_space->X_from_b = V2 * W_from_B;       // (N x N)
    }

    /**
     * @brief Default destructor.
     */
    ~StateSpaceCreator() noexcept = default;
};