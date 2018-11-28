
import ROOT
import numpy

ROOT.gSystem.Load('libEXOUtilities.so')
#ROOT.gSystem.Load('../lib/libEXOEnergy.so')

energyFitter = ROOT.EXOEnergyMCBasedFitEL()
energyFitter.SetVerboseLevel(2)
mcFileName = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/RealNoiseBugFix/SourceS5_Th228_fv_162_10_182_ss.root'
mcFile = ROOT.TFile(mcFileName,'read')
mcTree = mcFile.Get('mcTree')
mcTree.SetEstimate(mcTree.GetEntries()+1)
mcTree.Draw('energy_mc:weight:cluster_z[0]','','para goff')

energyFitter.AddMC('MC',0,0,0,0,0,mcTree.GetV1(),mcTree.GetV2(),mcTree.GetV3(),mcTree.GetSelectedRows())

mcFile.Close()

dataFileName = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data//WIPP/selection/2014_06_03/from_no_src_activity/mcbasedfit_2014v2both_run_3550_tree_fv_162_10_182_ss.root'
dataFile = ROOT.TFile(dataFileName,'read')
dataTree = dataFile.Get('dataTree')
dataTree.SetEstimate(dataTree.GetEntries()+1)
dataTree.Draw('e_charge:cluster_z[0]','','para goff')

energyFitter.AddData('data','MC',0,0,0,0,0,0,dataTree.GetV1(),dataTree.GetV2(),dataTree.GetSelectedRows())

dataFile.Close()

energyFitter.SplitZbins(numpy.array([0.,20.,50.,100.,200.]),4)

dataIds = ROOT.std.vector('TString')()
dataIds.push_back('data')
energyFitter.SetDataHisto('all','title',dataIds,125,1000,3500)

#calibFunc = ROOT.TF1('calib','[0]+[1]*x',0,3500,)
#calibFunc.SetParameters(0,1)
calibFunc = ROOT.TF2('calib','[0]+[1]*x*exp((115458 - 584.94*abs(y))/[2])',0,3500,0,250)
calibFunc.SetParameters(0,1,2000000.)
calibFunc.SetParError(2,500000.)
calibFunc.SetParError(0,10)
calibFunc.SetParError(1,0.1)

energyFitter.SetFunction('calib',calibFunc)

resolFunc = ROOT.TF1('resol','[0]',0,3500)
resolFunc.SetParameter(0,50.)
resolFunc.SetParError(0,10.)

energyFitter.SetFunction('resol',resolFunc)

energyFitter.BinMCEnergy(1)
energyFitter.BuildFitter()
energyFitter.RunFitter()

energyFitter.SaveHistosIn('test_elfit_split.root','recreate')
