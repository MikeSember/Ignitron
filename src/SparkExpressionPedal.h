#ifndef SPARK_EXPRESSION_PEDAL_H
#define SPARK_EXPRESSION_PEDAL_H

#include <Arduino.h>
#include "SparkStructures.h"

/**
 * Expression Pedal Handler for Ignitron
 * Features:
 * - Auto-detection of pedal insertion
 * - Automatic min/max value learning
 * - Pedal fully down = effect off
 * - Real-time parameter mapping
 */
class SparkExpressionPedal {
public:
    enum PedalState {
        DISCONNECTED,      // Not detected
        LEARNING,          // Calibrating min/max values
        ACTIVE,            // Ready to use
        LEARNING_COMPLETE  // Learning done, ready for normal operation
    };

    struct ExpressionTarget {
        uint8_t effectSlot;     // Effect index (0-6)
        uint8_t paramIndex;     // Parameter index within effect
        char effectName[32];    // Effect name for display
        char paramName[32];     // Parameter name for display
        float minValue;         // Minimum parameter value
        float maxValue;         // Maximum parameter value
    };

private:
    // Hardware
    uint8_t pedalPin;
    
    // Pedal state tracking
    PedalState state;
    int lastRawValue;
    int currentRawValue;
    unsigned long lastUpdateTime;
    unsigned long updateThreshold;  // Throttle updates (ms)
    
    // Auto-detection
    unsigned long lastActivityTime;
    static const unsigned long ACTIVITY_TIMEOUT = 30000;  // 30 seconds of no movement = unplugged
    static const int ADC_IDLE_THRESHOLD = 100;  // Variation threshold for idle detection
    bool isPluggedIn;
    
    // Calibration/Learning
    int calibMinRaw;
    int calibMaxRaw;
    int calibrationSamples;
    unsigned long calibrationStartTime;
    static const int CALIB_SAMPLE_COUNT = 100;
    static const unsigned long CALIB_TIMEOUT = 5000;  // 5 seconds to complete calibration
    
    // Pedal fully down detection (means effect is off)
    static const int FULL_DOWN_THRESHOLD = 200;  // ADC range: 0-4095
    
    // Expression target
    ExpressionTarget currentTarget;
    
    // Value history for smoothing
    int valueHistory[5];
    uint8_t historyIndex;
    
    // Pedal position for display
    float normalizedValue;
    
public:
    SparkExpressionPedal(uint8_t pin);
    ~SparkExpressionPedal();
    
    /**
     * Initialize and start auto-detection
     */
    void begin();
    
    /**
     * Main update function - call from main loop
     * @return true if parameter value changed and should be sent to Spark
     */
    bool update();
    
    /**
     * Check if pedal is currently plugged in
     */
    bool isConnected() const { return isPluggedIn; }
    
    /**
     * Get current pedal state
     */
    PedalState getState() const { return state; }
    
    /**
     * Get normalized pedal position (0.0 = full down/off, 1.0 = full up/max)
     * Returns 0.0 if pedal is fully down (effect off)
     */
    float getNormalizedValue() const;
    
    /**
     * Get the mapped parameter value based on calibrated min/max
     */
    float getMappedValue() const;
    
    /**
     * Set expression target from effect parameters
     * Automatically parses the effect slot and parameter index
     */
    void setTargetFromEffect(const SparkPreset& preset, uint8_t effectSlot, 
                            uint8_t paramIndex, const char* effectName, 
                            const char* paramName);
    
    /**
     * Get current target effect slot
     */
    uint8_t getTargetEffect() const { return currentTarget.effectSlot; }
    
    /**
     * Get current target parameter index
     */
    uint8_t getTargetParameter() const { return currentTarget.paramIndex; }
    
    /**
     * Get target effect name
     */
    const char* getTargetEffectName() const { return currentTarget.effectName; }
    
    /**
     * Get target parameter name
     */
    const char* getTargetParamName() const { return currentTarget.paramName; }
    
    /**
     * Start calibration/learning process
     */
    void startCalibration();
    
    /**
     * Check if calibration is in progress
     */
    bool isCalibrating() const { return state == LEARNING; }
    
    /**
     * Get calibration progress (0-100%)
     */
    uint8_t getCalibrationProgress() const;
    
    /**
     * Set expression target directly
     */
    void setTarget(uint8_t effectSlot, uint8_t paramIndex, 
                   const char* effectName, const char* paramName);
    
private:
    /**
     * Check if pedal is plugged in based on activity
     */
    void checkPedalConnection();
    
    /**
     * Process calibration samples
     */
    void processCalibraton();
    
    /**
     * Read and smooth analog value
     */
    int readPedalValue();
    
    /**
     * Get smoothed value from history
     */
    int getSmoothedValue() const;
};

#endif
