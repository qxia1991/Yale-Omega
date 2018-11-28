
import ROOT

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

infoRuns = ROOT.EXOSourceRunsPolishedInfo('../data/UpdatedLivetimeSourceRunsInfo.txt')

infoRuns.CutDefaultRuns()
#infoRuns.CutExact('TriggerPrescale','0')
#infoRuns.CutDoubleComparison('RunNumber',5900,False)

infoRuns.SetMetadata('DataTreeFileName1','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_Majoron/no_src_activity/mcbasedfit_2014v3both_run_[RunNumber]_tree.root')
infoRuns.SetMetadata('DataTreeFileName2','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2015_Cosmogenics/ForCalibration/cosmo_run_[RunNumber]_tree.root')

runNumbers = infoRuns.GetListOf('RunNumber')
fileNames1 = infoRuns.GetListOf('DataTreeFileName1')
fileNames2 = infoRuns.GetListOf('DataTreeFileName2')

fileNames = {}
for runNumber, fileName1, fileName2 in zip(runNumbers,fileNames1,fileNames2):
    run = int(runNumber)
    fileNames[run] = {}
    fileNames[run][1] = fileName1
    fileNames[run][2] = fileName2

ratioHisto = ROOT.TH1D('ratioHisto','',300,-1,2)
for run in sorted(fileNames):

    chain1 = ROOT.TChain('dataTree')
    chain2 = ROOT.TChain('dataTree')

    chain1.Add(fileNames[run][1])
    chain2.Add(fileNames[run][2])

    entries1 = chain1.GetEntries()
    entries2 = chain2.GetEntries()

    ratio = -0.5
    if entries2 > 0:
        ratio = entries2*1./entries1
    ratioHisto.Fill(ratio)
    print run, chain1.GetEntries(), chain2.GetEntries(), ratio

ratioHisto.Draw()
raw_input('end')
