
import ROOT

ROOT.gSystem.Load('libEXOCalibUtilities')

mcFile = ROOT.TFile(' /u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_04_14_calib/SourceS5_Th228_fv_162_10_182_ss.root','read')
mcTree = mcFile.Get('mcTree')
mcTree.SetEstimate(mcTree.GetEntries()+1)
mcTree.Draw('energy_mc:weight','','para goff')

resol = ROOT.EXOEnergyResol.GetInstanceForFlavor('2014-v1-average','energy-mcbased-fit')
h = resol.SmearedMC('Rotated',mcTree.GetV1(),mcTree.GetV2(),mcTree.GetSelectedRows(),1,300,0,3000,1346773897,0,1)

outFile = ROOT.TFile('hey.root','recreate')
h.Write()
outFile.Close()

del h
