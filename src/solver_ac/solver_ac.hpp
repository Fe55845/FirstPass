/**
 * ============================================================================
 *  Project        : FirstPass
 *  File           : solver_ac.hpp
 *
 *  Description
 *  --------------------------------------------------------------------------
 *  This file defines the SolverAc class, which solves the complex linear
 *  MNA matrix equation system (G * x = b) at each AC simulation frequency step
 *  using Sparse LU decomposition and streams the resulting node voltage phasors
 *  (magnitude and phase angle) to the output exporter.
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
 *       MnaSystemAc
 *            ↓
 *        SolverAc       <-- [This File]
 *            ↓
 *     ResultExporter
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
#include <fstream>
#include <string>

#include <eigen3/Eigen/Sparse>
#include <eigen3/Eigen/SparseLU>

#include "mna_system_ac.hpp"
#include "netlist.hpp"
#include "result_exporter.hpp"
#include "simulation_results.hpp"

/**
 * @brief Linear AC frequency-domain solver for MNA matrix systems using Sparse LU decomposition.
 */
class SolverAc {
private:
    /**
     * Pointer to the MNA equation structure containing matrices G, b, and solution vector x.
     */
    MnaSystemAc* mna_equation{nullptr};

    /**
     * Pointer to the current netlist configuration and node indexing mapping.
     */
    Netlist* netlist{nullptr};

    /**
     * Pointer to the simulation runtime metadata container.
     */
    SimulationResults* simulation_result{nullptr};

    /**
     * Pointer to the result exporter used for streaming CSV output data.
     */
    ResultExporter* output{nullptr};

public:
    /**
     * @brief Constructs the AC solver and writes the CSV header row to the output exporter.
     *
     * @param netlist Pointer to the circuit netlist.
     * @param mna_equation Pointer to the initialized complex MNA equations.
     * @param simulation_result Pointer to the destination result container.
     * @param output Pointer to the result exporter streaming destination.
     */
    SolverAc(
        Netlist* netlist,
        MnaSystemAc* mna_equation,
        SimulationResults* simulation_result,
        ResultExporter* output
    ) : mna_equation(mna_equation),
        netlist(netlist),
        simulation_result(simulation_result),
        output(output) {
        
        // Write CSV header row (frequency, node_1_abs, node_1_arg, ...)
        output->output << "frequency";
        for (int32_t i = 0; i < netlist->order_mna_equation; ++i) {
            output->output << "," << netlist->index_to_node_name[i] << "_abs"
                           << "," << netlist->index_to_node_name[i] << "_arg";
        }
    }

    /**
     * @brief Solves the complex linear MNA system G * x = b for a given frequency step.
     *
     * Decomposes the complex Sparse matrix G via SparseLU, solves for vector x, and
     * writes the resulting magnitude (|x|) and phase angle (arg(x)) to the output stream.
     *
     * @param frequency Current simulation frequency step in Hertz.
     * @param index Current frequency point iteration index.
     */
    void update_solution(double frequency, int index) {
        // Solve the linear complex system G * x = b using Sparse LU decomposition
        Eigen::SparseLU<Eigen::SparseMatrix<std::complex<double>>> solver;
        
        solver.compute(mna_equation->G);
        mna_equation->x = solver.solve(mna_equation->b);

        // Export magnitude and phase angle for each node/variable at the current frequency
        output->output << "\n" << frequency;

        for (Eigen::Index i = 0; i < mna_equation->x.rows(); ++i) {
            const std::complex<double>& val = mna_equation->x(i);
            output->output << "," << std::abs(val) << "," << std::arg(val);
        }
    }

    /**
     * @brief Default destructor.
     */
    ~SolverAc() = default;
};