using System.Windows;
using System.Windows.Input;
using BoardSimulator.ViewModels;

namespace BoardSimulator
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void BothPadsOn_Click(object sender, RoutedEventArgs e)
        {
            if (DataContext is MainViewModel vm)
            {
                vm.LeftFootpadVoltage = 3.3;
                vm.RightFootpadVoltage = 3.3;
            }
        }

        private void BothPadsOff_Click(object sender, RoutedEventArgs e)
        {
            if (DataContext is MainViewModel vm)
            {
                vm.LeftFootpadVoltage = 0.0;
                vm.RightFootpadVoltage = 0.0;
            }
        }

        private void PowerButton_MouseDown(object sender, MouseButtonEventArgs e)
        {
            if (DataContext is MainViewModel vm)
            {
                vm.IsButtonPressed = true;
            }
        }

        private void PowerButton_MouseUp(object sender, MouseButtonEventArgs e)
        {
            if (DataContext is MainViewModel vm)
            {
                vm.IsButtonPressed = false;
            }
        }

        private void PowerButton_MouseLeave(object sender, MouseEventArgs e)
        {
            // Release button if mouse leaves while pressed
            if (DataContext is MainViewModel vm)
            {
                vm.IsButtonPressed = false;
            }
        }
    }
}
