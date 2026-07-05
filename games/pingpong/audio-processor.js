class ImpactProcessor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.noiseFloor = 0.012;
    this.lastHitAt = 0;
    
    // Configurable settings
    this.sensitivityMultiplier = 1.0;
    
    this.port.onmessage = (event) => {
      if (event.data.type === 'set-sensitivity') {
        this.sensitivityMultiplier = event.data.value;
      }
    };
  }

  process(inputs, outputs, parameters) {
    const input = inputs[0];
    if (!input || !input.length) return true;
    const channel = input[0];
    if (!channel) return true;

    let sum = 0;
    let peak = 0;
    let zeroCrossings = 0;
    let previousSample = 0;

    for (let i = 0; i < channel.length; i++) {
      const sample = channel[i];
      sum += sample * sample;
      peak = Math.max(peak, Math.abs(sample));
      if ((sample >= 0) !== (previousSample >= 0)) zeroCrossings += 1;
      previousSample = sample;
    }

    const rms = Math.sqrt(sum / channel.length);
    if (rms < 0.06) {
      // Slower adaptation in the worklet due to much higher call rate (128 samples per block)
      this.noiseFloor = this.noiseFloor * 0.999 + rms * 0.001;
    }

    // Apply sensitivity multiplier (higher multiplier = lower threshold = more sensitive)
    const baseThreshold = Math.max(0.035, this.noiseFloor * 3.2);
    const threshold = baseThreshold / this.sensitivityMultiplier;
    
    const zeroCrossingRate = zeroCrossings / channel.length;
    const paddleImpact = peak > 0.11 && (peak / Math.max(rms, 0.001)) > 2.2 && zeroCrossingRate > 0.07;
    const now = currentTime * 1000;

    if (rms > threshold && paddleImpact && (now - this.lastHitAt > 300)) {
      this.lastHitAt = now;
      this.port.postMessage({ type: 'hit' });
    }

    return true;
  }
}

registerProcessor('impact-processor', ImpactProcessor);
