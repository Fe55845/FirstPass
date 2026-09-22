/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : solver_op.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the SolverOp class, which solves the real linear MNA
 *  matrix equation system (G * x = b) for DC Operating Point (.op) analysis
 *  using Sparse LU decomposition and exports the node voltages and branch
 *  currents to the output stream.
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
 *       MnaSystemOp
 *            ↓
 *        SolverOp       <-- [This File]
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

#include <eigen3/Eigen/Sparse>
#include <eigen3/Eigen/SparseLU>

#include "mna_system_op.hpp"
#include "netlist.hpp"
#include "result_exporter.hpp"
#include "simulation_results.hpp"

/**
 * @brief Linear DC/Static solver for passive linear circuits using Sparse LU decomposition.
 */
class SolverOp {
private:
    /**
     * Pointer to the MNA equation structure containing real matrices G, b, and solution vector x.
     */
    MnaSystemOp* mna_equation;

    /**
     * Pointer to the current netlist configuration and node indexing mapping.
     */
    Netlist* netlist;

    /**
     * Pointer to the simulation runtime metadata container.
     */
    SimulationResults* simulation_result;

    /**
     * Pointer to the result exporter used for streaming CSV output data.
     */
    ResultExporter* output;

public:
    /**
     * @brief Constructs the static DC solver, solves G * x = b, and exports the static state result.
     *
     * @param netlist Pointer to the circuit netlist.
     * @param mna_equation Pointer to the initialized real MNA equations.
     * @param simulation_result Pointer to the destination result object.
     * @param output Pointer to the result exporter streaming destination.
     */
    SolverOp(
        Netlist* netlist,
        MnaSystemOp* mna_equation,
        SimulationResults* simulation_result,
        ResultExporter* output
    ) : mna_equation(mna_equation),
        netlist(netlist),
        simulation_result(simulation_result),
        output(output) {

        // Solve the linear system G * x = b using Sparse LU decomposition
        Eigen::SparseLU<Eigen::SparseMatrix<double>> solver;
        solver.compute(mna_equation->G);
        mna_equation->x = solver.solve(mna_equation->b);

        // Write CSV header row (time, node_1, node_2, ...)
        output->output << "time";
        for (int32_t i = 0; i < netlist->order_mna_equation; ++i) {
            output->output << "," << netlist->index_to_node_name[i];
        }

        // Write static DC values at t = 0.0s
        output->output << "\n0.0";
        for (Eigen::Index i = 0; i < mna_equation->x.rows(); ++i) {
            output->output << "," << (mna_equation->x)(i);
        }
    }

    /**
     * @brief Default destructor.
     */
    ~SolverOp() = default;
};