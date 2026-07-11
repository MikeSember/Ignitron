#include "SparkExpressionPedal.h"
#include <string.h>
#include <algorithm>

SparkExpressionPedal::SparkExpressionPedal(uint8_t pin)
    : pedalPin(pin),
      state(DISCONNECTED),
      lastRawValue(-1),
      currentRawValue(0),
      lastUpdateTime(0),
      updateThreshold(50),
      lastActivityTime(0),
      isPluggedIn(false),
      calibMinRaw(4095),
      calibMaxRaw(0),
      calibrationSamples(0),
      normalizedValue(0.0f),
      historyIndex(0)
{
    memset(&currentTarget, 0, sizeof(ExpressionTarget));
    memset(valueHistory, 0, sizeof(valueHistory));
    // Default to Delay Volume
    strcpy(currentTarget.effectName, "Delay");
    strcpy(currentTarget.paramName, "Volume");
    currentTarget.effectSlot = 5;
    currentTarget.paramIndex = 0;
    currentTarget.minValue = 0.0f;
    currentTarget.maxValue = 1.0f;
}

SparkExpressionPedal::~SparkExpressionPedal() {
    // Cleanup if needed
}

void SparkExpressionPedal::begin() {
    pinMode(pedalPin, INPUT);
    lastActivityTime = millis();
    // Start auto-detection
    startCalibration();
}

int SparkExpressionPedal::readPedalValue() {
    return analogRead(pedalPin);
}

int SparkExpressionPedal::getSmoothedValue() const {
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += valueHistory[i];
    }
    return sum / 5;
}

void SparkExpressionPedal::checkPedalConnection() {
    int rawValue = readPedalValue();
    
    // Check for any activity (value changing)
    if (lastRawValue != -1 && abs(rawValue - lastRawValue) > ADC_IDLE_THRESHOLD) {
        lastActivityTime = millis();
        if (!isPluggedIn) {
            isPluggedIn = true;
            // Pedal just inserted - start calibration
            startCalibration();
        }
    }
    
    lastRawValue = rawValue;
    
    // Check for disconnection (no activity timeout)
    if (millis() - lastActivityTime > ACTIVITY_TIMEOUT) {
        if (isPluggedIn) {
            isPluggedIn = false;
            state = DISCONNECTED;
        }
    }
}

void SparkExpressionPedal::startCalibration() {
    state = LEARNING;
    calibMinRaw = 4095;
    calibMaxRaw = 0;
    calibrationSamples = 0;
    calibrationStartTime = millis();
}

void SparkExpressionPedal::processCalibraton() {
    if (state != LEARNING) return;
    
    int rawValue = readPedalValue();
    
    // Track min and max values
    calibMinRaw = std::min(calibMinRaw, rawValue);
    calibMaxRaw = std::max(calibMaxRaw, rawValue);
    calibrationSamples++;
    
    // Check if calibration is complete
    unsigned long calibElapsed = millis() - calibrationStartTime;
    bool enoughTime = calibElapsed >= CALIB_TIMEOUT;
    bool enoughSamples = calibrationSamples >= CALIB_SAMPLE_COUNT;
    
    if (enoughTime && enoughSamples) {
        // Ensure we have a valid range
        if (calibMaxRaw > calibMinRaw) {
            currentTarget.minValue = 0.0f;
            currentTarget.maxValue = 1.0f;
            state = ACTIVE;
        } else {
            // Invalid calibration, try again
            startCalibration();
        }
    }
}

float SparkExpressionPedal::getNormalizedValue() const {
    if (!isPluggedIn || state == DISCONNECTED) {
        return 0.0f;
    }
    
    // Pedal fully down = 0.0 (effect off)
    if (lastRawValue < FULL_DOWN_THRESHOLD) {
        return 0.0f;
    }
    
    // Map from calibrated range to 0.0-1.0
    int range = calibMaxRaw - calibMinRaw;
    if (range <= 0) return 0.0f;
    
    int adjusted = lastRawValue - calibMinRaw;
    float normalized = (float)adjusted / (float)range;
    
    // Clamp to 0.0-1.0
    return std::max(0.0f, std::min(1.0f, normalized));
}

float SparkExpressionPedal::getMappedValue() const {
    float normalized = getNormalizedValue();
    return currentTarget.minValue + (normalized * (currentTarget.maxValue - currentTarget.minValue));
}

uint8_t SparkExpressionPedal::getCalibrationProgress() const {
    if (state != LEARNING) return 100;
    
    unsigned long elapsed = millis() - calibrationStartTime;
    uint8_t timeProgress = (elapsed * 100) / CALIB_TIMEOUT;
    uint8_t sampleProgress = (calibrationSamples * 100) / CALIB_SAMPLE_COUNT;
    
    return std::max(timeProgress, sampleProgress);
}

void SparkExpressionPedal::setTarget(uint8_t effectSlot, uint8_t paramIndex,
                                    const char* effectName, const char* paramName) {
    currentTarget.effectSlot = effectSlot;
    currentTarget.paramIndex = paramIndex;
    currentTarget.minValue = 0.0f;
    currentTarget.maxValue = 1.0f;
    
    strncpy(currentTarget.effectName, effectName, sizeof(currentTarget.effectName) - 1);
    currentTarget.effectName[sizeof(currentTarget.effectName) - 1] = '\0';
    
    strncpy(currentTarget.paramName, paramName, sizeof(currentTarget.paramName) - 1);
    currentTarget.paramName[sizeof(currentTarget.paramName) - 1] = '\0';
}

void SparkExpressionPedal::setTargetFromEffect(const SparkPreset& preset, uint8_t effectSlot,
                                              uint8_t paramIndex, const char* effectName,
                                              const char* paramName) {
    setTarget(effectSlot, paramIndex, effectName, paramName);
    
    // Get min/max from effect parameters if available
    if (effectSlot < 7 && paramIndex < preset.effects[effectSlot].NumParameters) {
        // For "expr" parameters, range is 0.0-1.0
        currentTarget.minValue = 0.0f;
        currentTarget.maxValue = 1.0f;
    }
}

bool SparkExpressionPedal::update() {
    unsigned long now = millis();
    
    // Check pedal connection status
    checkPedalConnection();
    
    // Process calibration if active
    if (state == LEARNING) {
        processCalibraton();
    }
    
    // Throttle updates to prevent BLE flooding
    if (now - lastUpdateTime < updateThreshold) {
        return false;
    }
    
    if (!isPluggedIn || state == DISCONNECTED) {
        return false;
    }
    
    int rawValue = readPedalValue();
    
    // Add to history for smoothing
    valueHistory[historyIndex] = rawValue;
    historyIndex = (historyIndex + 1) % 5;
    
    int smoothedValue = getSmoothedValue();
    
    // Check if value changed significantly
    if (lastRawValue != -1 && abs(smoothedValue - lastRawValue) < 30) {
        return false;
    }
    
    lastRawValue = smoothedValue;
    currentRawValue = rawValue;
    lastUpdateTime = now;
    normalizedValue = getNormalizedValue();
    
    return true;
}
