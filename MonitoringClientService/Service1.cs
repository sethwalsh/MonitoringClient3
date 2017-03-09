using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Diagnostics;
using System.Linq;
using System.ServiceProcess;
using System.Text;
using System.Threading.Tasks;

using System.Threading;

namespace MonitoringClientService
{
    public partial class Service1 : ServiceBase
    {
        public Service1()
        {
            InitializeComponent();
        }

        protected override void OnStart(string[] args)
        {
            /// For now just create a new Client
            BackgroundWorker bw_ = new BackgroundWorker();
            bw_.DoWork += new DoWorkEventHandler(workerThreadFunction);
            bw_.RunWorkerAsync();
        }

        protected override void OnStop()
        {
            p.Kill();
        }

        private void workerThreadFunction(object sender, DoWorkEventArgs e)
        {
            p = new Process();
            p.StartInfo = new ProcessStartInfo("C:\\Tools\\Monitoring\\MonitoringClient.exe");
            p.WaitForExit();
            base.Stop();
        }

        private Process p;
    }
}
