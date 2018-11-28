
import ROOT

#ROOT.gInterpreter.GenerateDictionary("vector<TGraph>","TGraph.h;vector")

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

fv_wires,fv_min_z,fv_max_z = 162, 10, 182
cutName = 'fv_%i_%i_%i' % (fv_wires,fv_min_z,fv_max_z)

infoRuns = ROOT.EXOSourceRunsPolishedInfo('../data/UpdatedLivetimeSourceRunsInfo.txt')

#infoRuns.SetMetadata('DataTreeFileName','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_04_25_rhel6/no_src_activity/mcbasedfit_2014v1both_run_[RunNumber]_tree.root')
#infoRuns.SetMetadata('DataMultiplicityWildCard','[MULTIPLICITY]')
#infoRuns.SetMetadata('DataSelectionFileName','/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/WIPP/selection/2014_04_28_rhel6/from_no_src_activity/mcbasedfit_2014v1both_run_[RunNumber]_tree_%s_[MULTIPLICITY].root'%(cutName))

#infoRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/2014_04_10/') 
#infoRuns.SetMCSelectionFileNames(cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_04_14_calib/','[MULTIPLICITY]')

energyInspector = ROOT.EXOEnergyInspector(infoRuns,'2014-v3-channels-weekly')
energyInspector.SetOutputFile('testinspector_2014-v3-channels.root','recreate')
for channel in ['Ionization','Scintillation','Rotated']:
    energyInspector.PlotWeeklyParameters(channel,'Resolution')
    energyInspector.PlotWeeklyParameters(channel,'Calibration')
#energyInspector.PlotWeeklyParameters('Rotated','Resolution')
#energyInspector.PlotWeeklyParameters('Rotated','Calibration')
