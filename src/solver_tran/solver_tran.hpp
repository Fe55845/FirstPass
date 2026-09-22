/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : solver_tran.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the SolverTran class, which executes dynamic time-domain
 *  transient simulations (.tran) by discretizing the state-space representation
 *  (via matrix exponential) and streaming the node voltages and branch currents
 *  to the output exporter over time.
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
 *   StateSpaceModel
 *            ↓
 *       SolverTran      <-- [This File]
 *            ↓
 *     ResultExporter
 *
 *  Author         : Felix Zenhäusern
 *  Created        : 2026
 *  License        : MIT License
 * ============================================================================
 */

#pragma once

#include <cstdint>
#include <fstream>
#include <string>

#include <eigen3/Eigen/Dense>
#include <eigen3/unsupported/Eigen/MatrixFunctions>

#include "mna_system_tran.hpp"
#include "netlist.hpp"
#include "result_exporter.hpp"
#include "simulation_results.hpp"
#include "statespace_model_tran.hpp"

/**
 * @brief Dynamic time-domain solver using state-space discretization and matrix exponentials.
 */
class SolverTran {
private:
    /**
     * Pointer to the current netlist topology and simulation timing configuration.
     */
    Netlist* netlist;

    /**
     * Pointer to the transient MNA equation container.
     */
    MnaSystemTran* mna_equation;

    /**
     * Pointer to the extracted state-space representation system.
     */
    StateSpaceModel* state_space;

    /**
     * Pointer to the simulation runtime metadata container.
     */
    SimulationResults* simulation_result;

    /**
     * Pointer to the result exporter used for streaming CSV output data.
     */
    ResultExporter* output;

    using State  = Eigen::VectorXd;
    using Matrix = Eigen::MatrixXd;

public:
    /**
     * @brief Constructs the transient solver and executes time-stepping integration loop.
     *
     * Computes discrete state-transition matrix Ad = exp(F * dt) and input response gd,
     * then iterates through time steps streaming solution vector x to the result exporter.
     *
     * @param netlist Pointer to the circuit netlist configuration.
     * @param mna_equation Pointer to the MNA equation system.
     * @param state_space Pointer to the state-space model container.
     * @param simulation_result Pointer to destination simulation metadata object.
     * @param output Pointer to the result exporter streaming destination.
     */
    SolverTran(
        Netlist* netlist,
        MnaSystemTran* mna_equation,
        StateSpaceModel* state_space,
        SimulationResults* simulation_result,
        ResultExporter* output
    ) : netlist(netlist),
        mna_equation(mna_equation),
        state_space(state_space),
        simulation_result(simulation_result),
        output(output) {

        // Compute discrete state-transition matrix Ad = exp(F * dt)
        const Matrix Ad = (state_space->F * netlist->dt_sample).exp();

        const Matrix I = Matrix::Identity(
            state_space->F.rows(),
            state_space->F.cols()
        );

        // Solve discrete input transition vector gd = F^-1 * (Ad - I) * g
        const State gd = state_space->F.fullPivLu().solve(
            (Ad - I) * state_space->g
        );

        State z1 = State::Zero(netlist->order_state_space);

        // Write CSV header row (time, node_1, node_2, ...)
        output->output << "time";
        for (int32_t i = 0; i < netlist->order_mna_equation; ++i) {
            output->output << "," << netlist->index_to_node_name[i];
        }

        // Main time-stepping simulation loop
        for (uint32_t index = 0; index < netlist->n; ++index) {
            const double t = netlist->t_start + static_cast<double>(index) * netlist->dt_sample;

            output->output << "\n" << t;

            // Compute full state output vector x = X_z * z1 + X_b * b
            const State x = state_space->X_from_z * z1 + state_space->X_from_b * mna_equation->b;

            for (Eigen::Index i = 0; i < netlist->order_mna_equation; ++i) {
                output->output << "," << x(i);
            }

            // Update state vector for next time step: z1(k+1) = Ad * z1(k) + gd
            z1 = Ad * z1 + gd;
        }
    }

    /**
     * @brief Default destructor.
     */
    ~SolverTran() = default;
};