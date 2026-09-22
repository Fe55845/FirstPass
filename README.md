# FirstPass

FirstPass is an educational and research-oriented SPICE-like simulator for linear electrical networks. It uses Modified Nodal Analysis (MNA) to construct and solve operating-point, frequency-domain, and differential-algebraic circuit equations.

The project currently provides operating-point analysis, AC analysis, and experimental transient analysis. FirstPass is intended for education, numerical experimentation, and investigation of circuit-simulation algorithms. It is not intended to replace established production simulators.

## Project Status

FirstPass is under active development. The maturity of the individual analysis modes differs significantly.

### Operating Point

**Status: Implemented**

Operating-point analysis:

- constructs a real-valued sparse MNA system
- supports linear resistors and independent sources
- treats capacitors as open circuits
- treats inductors as ideal short circuits with additional branch-current variables
- solves the resulting system using Eigen's sparse LU decomposition
- is currently one of the more stable parts of the project

### AC Analysis

**Status: Implemented**

AC analysis:

- constructs a complex-valued sparse MNA system
- supports frequency-dependent resistor, capacitor, and inductor models
- performs a logarithmic frequency sweep
- exports magnitude and phase for every MNA unknown
- solves each frequency point using Eigen's sparse LU decomposition
- is currently one of the more stable parts of the project

The parser recognizes `dec`, `oct`, `lin`, and `list` AC sweep keywords. The current solver implementation, however, generates logarithmically spaced decade-style samples. The other parsed sweep modes should therefore be considered incomplete until their corresponding sampling algorithms are implemented.

### Transient Analysis

**Status: Experimental**

Transient analysis is a work in progress. It:

- constructs the dense differential MNA system
- supports selected known linear RLC circuits
- applies a full Singular Value Decomposition to separate dynamic and algebraic subspaces
- reduces the differential-algebraic system to a state-space model
- propagates the reduced state using a matrix exponential
- includes a deterministic regression test for the currently supported simulation path

The transient regression circuit is intentionally chosen to be compatible with the present implementation. A passing regression test confirms that this known path still produces the validated result. It does not imply support for every valid SPICE transient topology.

Transient analysis does not yet provide production-level numerical robustness. Some valid circuit topologies produce singular or poorly conditioned algebraic reduction blocks, and the current dense SVD approach is not suitable for large systems.

## Main Features

- SPICE-style text netlist parsing
- Translation of textual node names into numerical matrix indices
- Dedicated handling of the ground node `0`
- Engineering-prefix parsing
- Modified Nodal Analysis
- Sparse real-valued operating-point matrix assembly
- Sparse complex-valued AC matrix assembly
- Dense transient MNA matrix assembly
- Additional MNA variables for voltage-source currents
- Additional branch-current variables for inductors where required
- Sparse LU solution of operating-point and AC systems
- SVD-based state-space reduction for transient systems
- Matrix-exponential propagation of constant linear transient systems
- CSV result export
- Deterministic OP, AC, and transient testbench generation
- Numerical regression verification against stored reference output
- Optional visual comparison of selected results
- CMake-based C++17 build
- Shell scripts for release builds and test execution

## Supported Components

FirstPass currently supports the following linear two-terminal elements:

| Prefix | Component | Value |
|---|---|---|
| `R` | Resistor | Resistance in ohms |
| `C` | Capacitor | Capacitance in farads |
| `L` | Inductor | Inductance in henries |
| `I` | Independent current source | Current in amperes |
| `V` | Independent voltage source | Voltage in volts |

Component prefixes are accepted in uppercase or lowercase.

### Analysis-dependent behavior

#### Resistors

Resistors are represented by their conductance:

$$
Y_R = \frac{1}{R}
$$

They are stamped into the MNA system in all implemented analysis modes.

#### Capacitors

In operating-point analysis, capacitors are treated as open circuits.

In AC analysis, their complex admittance is:

$$
Y_C = j\omega C
$$

In transient analysis, capacitances are stamped into the dynamic matrix $C$.

#### Inductors

In operating-point analysis, inductors are treated as ideal short circuits. Each inductor introduces an additional branch-current unknown and is stamped similarly to a zero-volt voltage source.

In AC analysis, inductors are represented by:

$$
Y_L = \frac{1}{j\omega L}
$$

In transient analysis, each inductor introduces an additional branch-current variable. Its constitutive equation contributes to both the static matrix $G$ and the dynamic matrix $C$.

#### Independent current sources

Independent current sources are stamped directly into the right-hand-side vector. The current implementation treats the parsed source value as constant. Time-dependent current waveforms are not implemented.

#### Independent voltage sources

Independent voltage sources introduce additional MNA equations and branch-current unknowns.

The parser accepts:

- an implicit DC value
- an explicit `DC` value
- an `AC` magnitude and phase

Examples:

```text
V1 in 0 5
V2 bias 0 DC 2.5
V3 input 0 AC 1 0
```

The current transient implementation uses constant DC source values. Although additional waveform types exist as internal enumeration values, waveform parsing and simulation for `PULSE`, `SINE`, `EXP`, `SFFM`, and `PWL` are not implemented.

FirstPass does not currently implement nonlinear semiconductor devices, controlled sources, behavioral sources, models, or subcircuits.

## Supported Analysis Modes

### Operating-Point Analysis

An operating-point simulation is selected with:

```text
.op
```

The solver constructs the real-valued MNA system:

$$
Gx = b
$$

The unknown vector $x$ contains:

- voltages of all non-ground nodes
- currents through independent voltage sources
- inductor branch currents

Capacitors are omitted because they behave as open circuits under steady-state DC conditions. Inductors are represented as ideal short circuits with explicit branch-current equations.

The matrix is assembled as an `Eigen::SparseMatrix<double>` from triplets and solved using `Eigen::SparseLU`.

### AC Analysis

An AC analysis is selected with an `.ac` directive:

```text
.ac dec <points-per-decade> <start-frequency> <end-frequency>
```

At every frequency, FirstPass solves:

$$
G(j\omega)x = b
$$

where:

$$
\omega = 2\pi f
$$

The supported component admittances are:

$$
Y_R = \frac{1}{R}
$$

$$
Y_C = j\omega C
$$

$$
Y_L = \frac{1}{j\omega L}
$$

The complex sparse MNA matrix is rebuilt for each frequency because capacitor and inductor admittances depend on $\omega$. Each system is solved independently using sparse LU decomposition.

The output contains the magnitude and phase of every MNA unknown. Phase values are written in radians.

The parser recognizes `dec`, `oct`, `lin`, and `list`. At present, the simulation runner implements logarithmic decade-style frequency generation. `oct`, `lin`, and `list` should not yet be relied upon to produce their conventional SPICE sampling behavior.

### Transient Analysis

A transient simulation is selected with:

```text
.tran <start-time> <end-time>
```

The current parser interprets the first numerical argument as the start time and the second as the end time. It then creates a fixed number of output samples using the internal default sample count.

The transient MNA system is:

$$
C\dot{x} + Gx = b
$$

For general MNA systems, the dynamic matrix $C$ is singular. The equation is therefore a differential-algebraic equation rather than a conventional ordinary differential equation.

FirstPass computes a full Singular Value Decomposition:

$$
C = U\Sigma V^T
$$

The singular vectors are partitioned into dynamic and algebraic subspaces. The algebraic variables are eliminated to obtain a reduced model:

$$
\dot{z} = Fz + g
$$

The complete MNA solution is reconstructed with:

$$
x = X_{\text{from }z}z + X_{\text{from }b}b
$$

For a constant reduced system and a fixed sampling interval $\Delta t$, state propagation uses:

$$
A_d = e^{F\Delta t}
$$

and:

$$
z_{k+1} = A_dz_k + g_d
$$

The current implementation evaluates $g_d$ through a linear solve involving $F$ and $(A_d-I)g$.

Important transient limitations include:

- only constant source values are used
- initial conditions are not parsed from the netlist
- the reduced initial state is initialized to zero
- the complete MNA matrices are dense
- a full dense SVD is computed
- the algebraic reduction assumes an invertible algebraic coupling block
- some valid circuit topologies are not supported
- numerical behavior can be sensitive to topology, scale, rank decisions, and conditioning

## Mathematical Foundations

### Modified Nodal Analysis

Modified Nodal Analysis extends ordinary nodal analysis with additional unknowns for elements that cannot be represented using only nodal conductances.

In FirstPass, the unknown vector may contain:

- non-ground node voltages
- currents through independent voltage sources
- inductor branch currents

Conceptually:

$$
x =
\begin{bmatrix}
v_{\text{nodes}} \\
i_{\text{voltage sources}} \\
i_{\text{inductors}}
\end{bmatrix}
$$

The exact composition depends on the selected analysis mode.

Textual node names are translated into consecutive zero-based matrix indices. Ground is represented internally by the dedicated value `-1` and is not included as an ordinary matrix unknown.

### Transient SVD reduction

The transient matrix $C$ generally contains a nontrivial null space. FirstPass uses the decomposition $C = U\Sigma V^T$ to identify its numerical rank and split the system into a dynamic subspace and an algebraic subspace.

The algebraic subsystem is eliminated through the block:

$$
U_2^T G V_2
$$

The resulting reduced state-space model contains only the dynamic coordinates. This method is mathematically transparent and useful for studying descriptor systems, but the current implementation uses dense matrices and a full SVD. It is therefore primarily an educational and experimental method rather than a scalable large-circuit transient algorithm.

## Software Architecture

The primary simulation pipeline is:

```text
Input netlist
      |
      v
    Parser
      |
      v
    Netlist
      |
      v
SimulationRunner
      |
      +-- OP MNA builder   -> Sparse real solver
      |
      +-- AC MNA builder   -> Sparse complex solver
      |
      +-- TRAN MNA builder -> SVD state-space reduction
                              -> Matrix-exponential solver
      |
      v
 CSV result export
```

### Parser and internal netlist

`src/input/` contains the parser and the central netlist data model. The parser reads the input, ignores empty lines and comments, classifies components, translates node names, parses engineering prefixes, extracts analysis directives, and stores the result in a shared `Netlist`.

### Component representation

`src/components/` contains lightweight representations for passive two-terminal elements, independent current sources, and independent voltage sources with DC or AC parameters.

### Simulation dispatcher

`src/simulation_runner.hpp` selects the analysis backend and coordinates matrix allocation, MNA construction, solver creation, analysis execution, and result export.

### Analysis backends

Each analysis backend is separated into an MNA system container, an MNA builder, and a solver. Transient analysis also contains a reduced state-space model and the SVD-based state-space creator.

### Output subsystem

`src/output/` contains streamed CSV result generation and simulation metadata reporting. Results are written directly during simulation rather than retained as a complete in-memory result matrix.

## Repository Structure

```text
.
├── CMakeLists.txt
├── build_release.sh
├── examples
│   ├── ac_dense_large.net
│   ├── ac_dense_small.net
│   ├── op_dense_large.net
│   ├── op_dense_small.net
│   ├── rlc.net
│   └── tran_dense_large.net
├── src
│   ├── components
│   ├── input
│   ├── output
│   ├── solver_ac
│   ├── solver_op
│   ├── solver_tran
│   ├── main.cpp
│   └── simulation_runner.hpp
└── tests
    ├── ac
    ├── op
    ├── reference
    ├── tran
    ├── verifier
    ├── benchmark_results.csv
    └── run_testbench.sh
```

Generated build directories, compiled testbench generators, simulator outputs, plots, and other build artifacts may also be present in a working tree. They are not part of the logical source architecture.

The simulator is built from the files under `src/`.

## Requirements and Dependencies

FirstPass requires:

- a C++17-compatible compiler
- CMake 3.16 or newer
- Eigen 3
- a Unix-like shell for the supplied build and test scripts

On Debian or Ubuntu:

```bash
sudo apt update
sudo apt install build-essential cmake libeigen3-dev
```

On Arch Linux:

```bash
sudo pacman -S base-devel cmake eigen
```

Optional test and plotting workflows may additionally require tools used by the scripts in `tests/`, such as a C++ compiler and Gnuplot.

## Building FirstPass

### Release build script

```bash
chmod +x build_release.sh
./build_release.sh
```

The script uses plain `mkdir build`, so it expects that the directory does not already exist. For a clean rebuild:

```bash
rm -rf build
./build_release.sh
```

### Manual CMake build

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is generated at:

```text
build/FirstPass
```

The current target uses `-O3 -ffast-math -march=native`. The resulting binary may be specific to the build machine, and `-ffast-math` relaxes strict IEEE floating-point behavior.

## Running the Simulator

FirstPass expects exactly two command-line arguments:

```text
FirstPass <input-netlist> <output-file>
```

Example:

```bash
./build/FirstPass examples/rlc.net results.csv
```

If FirstPass is started without the required arguments, it prints an integrated help page.

## Netlist Syntax

### Components

```text
<name> <positive-node> <negative-node> <value>
```

Examples:

```text
R1 in out 1k
C1 out 0 10u
L1 out load 1m
I1 0 in 2m
V1 in 0 5
```

Element type is determined by the first character of the component name. Node `0` is reserved for ground.

### Comments

Lines whose first non-whitespace character is `*` or `;` are ignored. Inline comments are not explicitly parsed.

### Engineering prefixes

| Suffix | Multiplier |
|---|---:|
| `G`, `g` | $10^9$ |
| `Meg`, `meg`, `MEG` | $10^6$ |
| `k`, `K` | $10^3$ |
| no suffix | $1$ |
| `m`, `M` | $10^{-3}$ |
| `u`, `U` | $10^{-6}$ |
| `n`, `N` | $10^{-9}$ |
| `p`, `P` | $10^{-12}$ |
| `f`, `F` | $10^{-15}$ |

`M` is treated as milli rather than mega. Use `Meg` for $10^6$.

### Directives

Operating point:

```text
.op
```

AC analysis:

```text
.ac dec <points-per-decade> <start-frequency> <end-frequency>
```

Transient analysis:

```text
.tran <start-time> <end-time>
```

Unknown directives are ignored. `.dc` is recognized as an analysis type, but a DC sweep solver is not implemented. `.print` and `.end` may appear in netlists, but currently do not filter output or terminate parsing.

## Output Format

### Operating point

```text
time,node1,node2,i_V1
0.0,5,2.5,-0.0025
```

### AC analysis

```text
frequency,in_abs,in_arg,out_abs,out_arg,i_V1_abs,i_V1_arg
```

Magnitude is the complex absolute value. Phase is written in radians.

### Transient analysis

```text
time,node1,node2,i_L1,i_V1
```

FirstPass currently exports all MNA unknowns. Parsed `.print` directives do not filter the output columns.

## Examples

Operating point:

```bash
./build/FirstPass examples/op_dense_small.net op_results.csv
```

```text
* Resistive divider
V1 in 0 DC 10
R1 in out 1k
R2 out 0 1k
.op
.end
```

AC analysis:

```bash
./build/FirstPass examples/ac_dense_small.net ac_results.csv
```

```text
* RC low-pass filter
V1 in 0 AC 1 0
R1 in out 1k
C1 out 0 1u
.ac dec 100 10 1Meg
.end
```

Transient analysis:

```bash
./build/FirstPass examples/rlc.net tran_results.csv
```

```text
* Experimental RLC transient analysis
V1 in 0 1
R1 in n1 1k
L1 n1 out 1m
C1 out 0 1u
.tran 0 0.3
.end
```

Transient support is topology-dependent. Begin with the supplied examples and regression test before constructing more complex transient systems.

## Testing and Validation

The test infrastructure is organized by analysis mode:

```text
tests/op/
tests/ac/
tests/tran/
tests/reference/
tests/verifier/
```

The repository contains deterministic C++ testbench generators for operating-point resistor networks, AC RLC networks, and a supported transient RLC network. The validated reference results are provided as downloadable assets through the corresponding GitHub Release. After downloading and extracting them into `tests/reference/`, the numerical verifier can compare newly generated output against the stored reference data.

Run the complete test workflow from the repository root:

```bash
chmod +x tests/run_testbench.sh
./tests/run_testbench.sh
```

The transient regression test is deliberately based on a circuit known to work with the current SVD reduction. A passing result confirms the validated path, not arbitrary descriptor-system or RLC-topology support.

Floating-point results may vary slightly depending on the compiler version, Eigen version, processor architecture, sparse matrix ordering, and optimization settings. The provided numerical verifier accounts for small floating-point deviations by comparing generated results against the validated reference data using numerical tolerances rather than exact textual equality. The verifier must be built before running the testbench and is executed automatically by `run_testbench.sh`.

## External Simulator Comparison

The repository contains CSV and plotting artifacts that support comparison with externally generated simulator data. These comparisons are useful for checking trends, validating representative node voltages, comparing AC magnitude and phase, identifying regressions, and investigating numerical disagreement.

External comparison is a validation aid, not a claim of complete compatibility or numerical equivalence with another simulator. Comparisons should use the same circuit interpretation, source values, frequency points, initial conditions, and output variables.

## Benchmarks

Publication-ready benchmark results are not currently available.

Preliminary development measurements indicate that operating-point analysis is in a good performance state. On some large generated linear test cases, FirstPass has shown approximately 10x to 100x shorter runtimes than common simulators tested during development, including ngspice, Xyce, and LTspice. These observations have not yet been converted into a controlled, reproducible benchmark study and should not be interpreted as general performance claims.

AC performance is also promising, but depends strongly on the circuit and frequency sweep. Current AC performance can be limited by netlist parsing and CSV output generation. For small cases, FirstPass may perform similarly to traditional simulators because startup, parsing, and output costs dominate the linear solve.

Transient analysis is still in development and is not currently suitable for meaningful comparative benchmarking. Its dense matrices, full SVD, and numerical sensitivity dominate runtime and scalability.

Current development observations suggest that OP and AC numerical stability are broadly comparable to tested ngspice and Xyce cases, while LTspice is significantly more robust on difficult numerical cases. Transient numerical stability remains an active development area.

## Current Limitations

### General

- No complete SPICE compatibility
- Linear components only
- No nonlinear devices, controlled sources, behavioral sources, subcircuits, or model cards
- No implemented DC sweep solver
- No `.print`-based output filtering
- Limited syntax validation and diagnostics
- Unknown elements and directives may be ignored
- Solver status is not comprehensively reported
- Singular and ill-conditioned systems may produce invalid results

### Parser and AC analysis

- Element type is inferred from the first character of its name
- Inline comments are not explicitly supported
- AC phase values are interpreted as radians
- AC source lines should provide magnitude and phase
- `M` means milli; use `Meg` for mega
- Conventional SPICE `.tran` variants are not fully supported
- `.dc` is parsed but not solved
- `oct`, `lin`, and `list` are parsed but lack matching sampling implementations
- The requested end frequency is not guaranteed to be sampled exactly
- A new sparse decomposition is performed for every frequency

### Operating point

- Only linear equations are supported
- Singular circuits are not automatically regularized
- Floating nodes and contradictory ideal sources may lead to an unsolvable matrix
- Sparse solver failures are not yet exposed through detailed diagnostics

### Transient analysis

- Experimental and topology-dependent
- Dense $G$ and $C$ matrices
- Full dense SVD
- Limited scalability
- Constant sources only
- No time-dependent waveform parsing
- No user-defined initial conditions
- Zero initial reduced state
- Fixed output sample count
- No adaptive integration or error control
- No event or discontinuity handling
- No sparse descriptor-system reduction
- No robust treatment of every singular algebraic block
- No general higher-index DAE reduction
- Numerical rank selection may be scale-sensitive
- The input discretization can be unreliable when $F$ is singular or poorly conditioned

## Development Principles

FirstPass prioritizes:

1. **Readability**
2. **Maintainability**
3. **Mathematical transparency**
4. **Educational value**
5. **Deterministic testing**

These goals take precedence over complete SPICE coverage, maximum performance, support for every valid topology, and industrial numerical robustness.

## Feedback and Contributions

FirstPass is currently developed as a single-author educational and research project. A central goal of the project is to learn and document the implementation of a linear circuit simulator by developing its numerical methods, architecture, and supporting tools directly.

For this reason, external code contributions and pull requests are not currently being accepted. Technical feedback, bug reports, numerical observations, and suggestions for possible improvements are nevertheless welcome through GitHub Issues.

The project is released under the MIT License, and users are welcome to fork the repository and develop independent modifications or extensions.

## License

FirstPass is released under the MIT License. See the repository's license file for the complete license text.

## Author

**Felix Zenhäusern**
