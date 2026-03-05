using System;
using System.Globalization;
using System.Windows.Data;

namespace BoardSimulator.Converters
{
    /// <summary>
    /// Converts brightness value (0-100) to opacity (0-1) for glow effects
    /// </summary>
    public class BrightnessToOpacityConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, CultureInfo culture)
        {
            if (value is double brightness)
            {
                // Convert 0-100 brightness to 0-1 opacity
                return Math.Max(0.0, Math.Min(1.0, brightness / 100.0));
            }
            return 0.0;
        }

        public object ConvertBack(object value, Type targetType, object parameter, CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
