using System.Windows.Media;

namespace BoardSimulator.ViewModels
{
    public class StatusLedViewModel : ViewModelBase
    {
        private bool _isOn;
        private double _brightness;
        private Color _color;

        public bool IsOn
        {
            get => _isOn;
            set
            {
                if (SetProperty(ref _isOn, value))
                {
                    OnPropertyChanged(nameof(DisplayBrush));
                }
            }
        }

        public double Brightness
        {
            get => _brightness;
            set
            {
                if (SetProperty(ref _brightness, value))
                {
                    OnPropertyChanged(nameof(DisplayBrush));
                }
            }
        }

        public Color Color
        {
            get => _color;
            set
            {
                if (SetProperty(ref _color, value))
                {
                    OnPropertyChanged(nameof(DisplayBrush));
                }
            }
        }

        public Brush DisplayBrush
        {
            get
            {
                if (!IsOn)
                {
                    // Dim color when off
                    return new SolidColorBrush(System.Windows.Media.Color.FromRgb(
                        (byte)(Color.R * 0.1),
                        (byte)(Color.G * 0.1),
                        (byte)(Color.B * 0.1)));
                }

                // Apply brightness
                double factor = Brightness / 100.0;
                return new SolidColorBrush(System.Windows.Media.Color.FromRgb(
                    (byte)(Color.R * factor),
                    (byte)(Color.G * factor),
                    (byte)(Color.B * factor)));
            }
        }

        public StatusLedViewModel(Color color)
        {
            _color = color;
            _isOn = false;
            _brightness = 100;
        }
    }
}
