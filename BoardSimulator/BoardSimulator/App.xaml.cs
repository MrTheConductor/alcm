using System.Windows;

namespace BoardSimulator
{
    /// <summary>
    /// Interaction logic for App.xaml
    /// </summary>
    public partial class App : Application
    {
        protected override void OnStartup(StartupEventArgs e)
        {
            base.OnStartup(e);
            
            // Any app-wide initialization can go here
        }
        
        protected override void OnExit(ExitEventArgs e)
        {
            // Cleanup on exit
            base.OnExit(e);
        }
    }
}
