/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : statespace_model_tran.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the StateSpaceModel class, which acts as a lightweight
 *  data container for storing the reduced-order state-space system matrices
 *  (F, g, X_from_z, X_from_b) derived from transient MNA equations.
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
 *  StateSpaceCreator
 *            ↓
 *   StateSpaceModel     <-- [This File]
 *            ↓
 *       SolverTran
 *
 *  Mathematical Background
 *  --------------------------------------------------------------------------
 *  Differential State Equation:
 *      dz1/dt = F * z1 + g
 *
 *  Output Recovery Equation (MNA unknowns vector x):
 *      x = X_from_z * z1 + X_from_b * b
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <eigen3/Eigen/Dense>

/**
 * @brief Container class for storing reduced-order state-space system matrices.
 */
class StateSpaceModel {
public:
    /**
     * System state transition matrix F (size r x r, where r is state-space rank).
     */
    Eigen::MatrixXd F;

    /**
     * Input response excitation vector g (size r x 1).
     */
    Eigen::VectorXd g;

    /**
     * Output projection matrix mapping reduced state z1 to full MNA vector x (size N x r).
     */
    Eigen::MatrixXd X_from_z;

    /**
     * Output projection matrix mapping input source vector b to full MNA vector x (size N x N).
     */
    Eigen::MatrixXd X_from_b;

    /**
     * @brief Default constructor.
     */
    StateSpaceModel() noexcept = default;

    /**
     * @brief Default destructor.
     */
    ~StateSpaceModel() noexcept = default;
};