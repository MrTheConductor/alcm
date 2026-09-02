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
