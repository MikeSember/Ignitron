#ifndef SPARK_WAH_PEDAL_H
#define SPARK_WAH_PEDAL_H

#include <Arduino.h>
#include "SparkExpressionPedal.h"
#include "SparkStructures.h"

/**
 * Wah Pedal Controller
 * Converts expression pedal input to wah effect parameters
 * Uses the Modulation slot (slot 4) to host wah effects
 */
class SparkWahPedal {
public:
    enum WahMode {
        WAH_OFF,
        WAH_ACTIVE
    };
    
    struct WahConfig {
        char effectName[32];        // Wah effect model (e.g., "Vox Wah")
        float centerFreq;           // Center frequency parameter
        float resonance;            // Q/resonance parameter
        uint8_t freqParamIndex;     // Parameter index for frequency
        uint8_t resParamIndex;      // Parameter index for resonance
    };

private:
    SparkExpressionPedal* expressionPedal;
    WahMode mode;
    WahConfig config;
    bool isActive;
    float lastWahFreq;
    
    // Wah sweep parameters
    static const float MIN_WAH_FREQ;     // Low frequency (heel down)
    static const float MAX_WAH_FREQ;     // High frequency (toe up)
    static const float MIN_RESONANCE;
    static const float MAX_RESONANCE;
    
public:
    SparkWahPedal(SparkExpressionPedal* pedal);
    
    /**
     * Initialize wah with specific effect
     * Searches for wah-like effects in preset
     */
    void begin(const SparkPreset& preset);
    
    /**
     * Update wah parameters based on expression pedal
     * @return true if parameters changed
     */
    bool update();
    
    /**
     * Activate wah mode
     */
    void activate();
    
    /**
     * Deactivate wah mode
     */
    void deactivate();
    
    /**
     * Check if wah is active
     */
    bool isWahActive() const { return isActive; }
    
    /**
     * Get current wah mode
     */
    WahMode getMode() const { return mode; }
    
    /**
     * Get current frequency value
     */
    float getCurrentFrequency() const;
    
    /**
     * Get effect name
     */
    const char* getEffectName() const { return config.effectName; }
    
    /**
     * Set wah effect (by name)
     * Common wah effects: "Vox Wah", "Fuzz Wah", "Cry Wah", etc.
     */
    void setWahEffect(const char* effectName, uint8_t freqParam, uint8_t resParam);
    
private:
    /**
     * Calculate wah frequency sweep based on pedal position
     */
    float calculateWahFrequency();
    
    /**
     * Calculate resonance/Q parameter
     */
    float calculateResonance();
};

#endif
