# FirstPass Roadmap

This document collects potential improvements and development directions for FirstPass.

The roadmap is exploratory. It does not represent a fixed release schedule, and listed items may be changed, postponed, replaced, or removed as the project evolves.

## Near-Term Priorities

- Improve parser validation and error reporting
- Add explicit solver-status checks
- Improve diagnostics for singular and ill-conditioned systems
- Extend deterministic regression coverage

## Operating-Point Analysis

- Improve reporting of singular systems
- Add clearer diagnostics for floating nodes
- Investigate additional sparse solver strategies

## AC Analysis

- Implement linear, octave, and list-based frequency sweeps
- Ensure exact inclusion of requested sweep endpoints
- Investigate reuse of sparse matrix structure between frequency points
- Improve result export performance

## Transient Analysis

- Improve numerical rank selection
- Investigate singular or rank-deficient algebraic coupling blocks
- Replace or improve the current input discretization when the reduced system matrix is singular
- Add support for time-dependent independent sources
- Add initial-condition handling
- Investigate sparse descriptor-system reduction
- Investigate adaptive integration methods

## Parser and Netlist Support

- Add structured parser diagnostics
- Report unknown components and directives
- Improve validation of source syntax
- Implement selective output based on `.print`
- Consider DC sweep support

## Testing and Tooling

- Automate downloading of reference results
- Automatically build missing test dependencies
- Improve cross-platform test execution
- Add continuous integration
- Expand external simulator comparisons
- Consider a reproducible benchmark suite

## Non-Goals

FirstPass is not intended to become a complete replacement for established production circuit simulators. Complete SPICE compatibility and support for every device model are not current project goals.