using NAudio.Wave;
using NAudio.Wave.SampleProviders;
using System;

namespace BoardSimulator.Services
{
    /// <summary>
    /// Service for generating buzzer tones
    /// </summary>
    public class BuzzerService : IDisposable
    {
        private IWavePlayer? _waveOut;
        private SignalGenerator? _signalGenerator;
        private bool _isPlaying;
        private ushort _currentFrequency;

        public BuzzerService()
        {
            // Initialize audio output
            _waveOut = new WaveOutEvent();
            _signalGenerator = new SignalGenerator(44100, 1)
            {
                Type = SignalGeneratorType.Square,
                Gain = 0.2 // 20% volume to avoid being too loud
            };
        }

        /// <summary>
        /// Start or update the buzzer tone
        /// </summary>
        /// <param name="frequency">Frequency in Hz (0 = stop)</param>
        public void SetTone(ushort frequency)
        {
            if (frequency == 0)
            {
                Stop();
                return;
            }

            // If already playing the same frequency, do nothing
            if (_isPlaying && _currentFrequency == frequency)
            {
                return;
            }

            // Update frequency
            _currentFrequency = frequency;

            if (_signalGenerator != null)
            {
                _signalGenerator.Frequency = frequency;

                if (!_isPlaying)
                {
                    // Start playing
                    if (_waveOut != null)
                    {
                        _waveOut.Init(_signalGenerator);
                        _waveOut.Play();
                        _isPlaying = true;
                    }
                }
            }
        }

        /// <summary>
        /// Stop the buzzer
        /// </summary>
        public void Stop()
        {
            if (_isPlaying && _waveOut != null)
            {
                _waveOut.Stop();
                _isPlaying = false;
                _currentFrequency = 0;
            }
        }

        public void Dispose()
        {
            Stop();
            _waveOut?.Dispose();
            _waveOut = null;
            _signalGenerator = null;
        }
    }
}
