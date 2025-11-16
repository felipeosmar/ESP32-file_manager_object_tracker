/**
 * Motion Detection Implementation
 */

#include "motion_detector.h"
#include <cstring>

MotionDetector::MotionDetector()
  : previousFrame(nullptr), frameSize(0), threshold(30), initialized(false) {
}

MotionDetector::~MotionDetector() {
  if (previousFrame) {
    free(previousFrame);
  }
}

void MotionDetector::begin(int motionThreshold) {
  threshold = motionThreshold;
  memset(sampledPrevious, 0, sizeof(sampledPrevious));
  initialized = false;
  Serial.println("Motion detector initialized");
}

void MotionDetector::setThreshold(int newThreshold) {
  threshold = newThreshold;
}

Point MotionDetector::detectMotion(uint8_t *currentFrame, int width, int height) {
  // Downsample current frame for faster processing
  downsampleFrame(currentFrame, width, height, sampledCurrent);

  if (!initialized) {
    // First frame - just store it
    memcpy(sampledPrevious, sampledCurrent, sizeof(sampledCurrent));
    initialized = true;
    return Point(-1, -1);
  }

  // Calculate difference between current and previous frame
  uint8_t diff[SAMPLE_WIDTH * SAMPLE_HEIGHT];
  int motionPixels = 0;

  for (int i = 0; i < SAMPLE_WIDTH * SAMPLE_HEIGHT; i++) {
    int pixelDiff = abs(sampledCurrent[i] - sampledPrevious[i]);
    if (pixelDiff > threshold) {
      diff[i] = 255;
      motionPixels++;
    } else {
      diff[i] = 0;
    }
  }

  // Update previous frame
  memcpy(sampledPrevious, sampledCurrent, sizeof(sampledCurrent));

  // If enough motion detected, calculate centroid
  if (motionPixels > 50) { // Minimum motion threshold
    Point center = calculateCentroid(diff);

    // Scale back to original resolution
    if (center.x != -1 && center.y != -1) {
      center.x = (center.x * width) / SAMPLE_WIDTH;
      center.y = (center.y * height) / SAMPLE_HEIGHT;
    }

    return center;
  }

  return Point(-1, -1);
}

void MotionDetector::downsampleFrame(uint8_t *frame, int width, int height, uint8_t *output) {
  // Simple downsampling by averaging blocks
  int blockWidth = width / SAMPLE_WIDTH;
  int blockHeight = height / SAMPLE_HEIGHT;

  for (int y = 0; y < SAMPLE_HEIGHT; y++) {
    for (int x = 0; x < SAMPLE_WIDTH; x++) {
      // Sample center of each block
      int srcX = x * blockWidth + blockWidth / 2;
      int srcY = y * blockHeight + blockHeight / 2;

      // For JPEG, we'll use a simplified approach
      // In production, you might want to decode to RGB/Grayscale first
      // For now, sample the luminance directly from buffer
      int idx = srcY * width + srcX;
      if (idx < width * height) {
        output[y * SAMPLE_WIDTH + x] = frame[idx] & 0xFF;
      }
    }
  }
}

Point MotionDetector::calculateCentroid(uint8_t *diffFrame) {
  long sumX = 0;
  long sumY = 0;
  long count = 0;

  for (int y = 0; y < SAMPLE_HEIGHT; y++) {
    for (int x = 0; x < SAMPLE_WIDTH; x++) {
      int idx = y * SAMPLE_WIDTH + x;
      if (diffFrame[idx] > 0) {
        sumX += x;
        sumY += y;
        count++;
      }
    }
  }

  if (count > 0) {
    return Point(sumX / count, sumY / count);
  }

  return Point(-1, -1);
}
