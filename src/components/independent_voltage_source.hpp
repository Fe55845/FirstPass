#pragma once



#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>


/**
 * @brief Defines the waveform type of an independent voltage source.
 *
 * Only a subset may currently be implemented by the simulator. The remaining
 * modes already exist to simplify future extensions.
 */
enum class SourceMode : uint8_t {
    AC    = 0,
    DC    = 1,
    PULSE = 2,
    SINE  = 3,
    EXP   = 4,
    SFFM  = 5,
    PWL   = 6
};

/**
 * @brief Represents an independent voltage source.
 *
 * Voltage sources are stored separately from passive components because
 * they support additional excitation models and parameters that are not
 * required by standard passive elements.
 *
 * Examples:
 *
 *      DC sources
 *      AC sources
 *      SINE sources
 *      PULSE sources
 *
 * While passive components can be represented using a simple
 * (name, node_a, node_b, value) structure, voltage sources require
 * additional information such as waveform type, DC offset, amplitude
 * and phase.
 */
class IndependentVoltageSource {
public:

    /**
     * Source identifier.
     *
     * Examples:
     *
     *      V1
     *      Vin
     *      Vreference
     */
    std::string name;

    /**
     * Positive source terminal.
     */
    int32_t positive_node;

    /**
     * Negative source terminal.
     */
    int32_t negative_node;

    /**
     * DC source value.
     *
     * Example:
     *
     *      V1 in 0 DC 5
     */
    double dc_offset;

    /**
     * AC magnitude.
     *
     * Example:
     *
     *      V1 in 0 AC 1
     */
    double amplitude;

    /**
     * AC phase angle in radians.
     */
    double phase;

    /**
     * Source waveform model.
     */
    SourceMode source_mode;

    /**
     * @brief Construct a new Independent Voltage Source object.
     *
     * @param name Voltage source identifier.
     * @param positive_node Positive terminal node index.
     * @param negative_node Negative terminal node index.
     * @param source_mode Source waveform type.
     */
    IndependentVoltageSource(
        std::string name,
        int32_t positive_node,
        int32_t negative_node,
        SourceMode source_mode
    )
        : name(name),
          positive_node(positive_node),
          negative_node(negative_node),
          source_mode(source_mode)
    {
        // Initialize all voltage sources with a zero DC offset.
        dc_offset = 0.0;
    }
};