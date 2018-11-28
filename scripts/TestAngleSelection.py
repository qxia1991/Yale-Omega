
import ROOT
import numpy

ROOT.gSystem.Load('../lib/libEXOEnergy.so')


fv_wires,fv_min_z,fv_max_z = 162, 10, 182
cutName = 'fv_%i_%i_%i' % (fv_wires,fv_min_z,fv_max_z)

infoRuns = ROOT.EXOSourceRunsPolishedInfo('../data/UpdatedLivetimeSourceRunsInfo.txt')
infoRuns.CutDefaultRuns()
infoRuns.SetMetadata('DataTreeFileName','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_Majoron/calib_channels_no_src_veto/mcbasedfit_2014v3both_2014v3channels_run_[RunNumber]_tree.root')
infoRuns.SetMetadata('DataMultiplicityWildCard','[MULTIPLICITY]')
infoRuns.SetMetadata('DataSelectionFileName','/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/WIPP/selection/2014_Majoron/from_calib_channels_no_src_veto/mcbasedfit_2014v3both_2014v3channels_run_[RunNumber]_tree_%s_[MULTIPLICITY].root'%(cutName))
infoRuns.SetMetadata('DataId','mcbasedfit_2014v3both_2014v3channels_run_[RunNumber]_[MULTIPLICITY]')

infoRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/2014_Majoron/')
infoRuns.SetMCSelectionFileNames(cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_Majoron/','[MULTIPLICITY]')

#only Th228 at S5
infoRuns.CutExact('SourcePositionS','5')
infoRuns.CutExact('SourcePositionZ','0.0')#,False)
infoRuns.CutExact('SourceName','Th-228')

sourceX = infoRuns.GetListOf('SourcePositionX')
sourceX = float(sourceX.at(0))*10.
sourceY = infoRuns.GetListOf('SourcePositionY')
sourceY = float(sourceY.at(0))*10.
sourceZ = infoRuns.GetListOf('SourcePositionZ')
sourceZ = float(sourceZ.at(0))*10.

evtSum = ROOT.EXOEventSummary()

mcFiles = infoRuns.GetListOf('MCSelectionFileName')
dataFiles = infoRuns.GetSetOf('DataSelectionFileName')
#mcTree = ROOT.TChain('dataTree')
#for dataFileName in dataFiles:
#    mcTree.Add(dataFileName.replace('[MULTIPLICITY]','ms'))
#mcTree.Scan("eventNum:Row","(2590 < energy_ms && energy_ms < 2650) && @cluster_energy.size() == 2")

mcFile = ROOT.TFile(str(mcFiles[0]).replace('[MULTIPLICITY]','ms'),'read')
mcTree = mcFile.Get('mcTree')

mcTree.SetBranchAddress('EventSummary',evtSum)

mcTree.Draw(">>elist","(2590 < energy_mc && energy_mc < 2650) && @cluster_energy.size() == 2","entrylist")
elist = ROOT.gDirectory.Get("elist")
#print elist.GetN()
mcTree.SetEntryList(elist)

cF = ROOT.TF1('cf','1./(1/2614.5 + (1-cos(x))/512.)',0,ROOT.TMath.Pi())
resolDB = ROOT.EXOEnergyResol.GetInstanceForFlavor("2014-v3-channels-average","energy-mcbased-fit")
resolF = ROOT.TF1('resol',resolDB.ResolutionString("Ionization","x",1,1316363203,0),0,10000)

vecMax = ROOT.TVector3(0,0,0)
vecSec = ROOT.TVector3(0,0,0)
hEvsA = ROOT.TH2D('hEvsA','',1000,0,ROOT.TMath.Pi(),3500,0,3500)
hEsel = ROOT.TH1D('hEsel','',3500,0,3500)
hEpull = ROOT.TH1D('hEpull','',20000,-100,100)
hEdiff = ROOT.TH1D('hEdiff','',3500,0,3500)
for i in range(elist.GetN()):#mcTree.GetEntries()):

    #if i > 1000:
    #    break
    if i % 100000 == 0:
        print i, 'of', elist.GetN()#mcTree.GetEntries()

    entryNumber = mcTree.GetEntryNumber(i)
    if entryNumber < 0:
        break
    #localEntry = mcTree.LoadTree(entryNumber)
    #if localEntry < 0:
    #    break
    #print i, elist.GetEntry(i), entryNumber#elist.Next()
    mcTree.GetEntry(entryNumber)
    ncl = evtSum.cluster_energy.size()

    if evtSum.cluster_energy[0] < 150 or evtSum.cluster_energy[1] < 150:
        continue


    maxIdx = 0 
    for j in range(ncl):
        if evtSum.cluster_energy.at(maxIdx) < evtSum.cluster_energy.at(j):
            maxIdx = j
    secIdx = (0,1)[maxIdx == 0]
    for j in range(ncl):
        if j != maxIdx and evtSum.cluster_energy.at(secIdx) < evtSum.cluster_energy.at(j):
            secIdx = j
    #print maxIdx, evtSum.cluster_energy.at(maxIdx), secIdx, evtSum.cluster_energy.at(secIdx)
    #print 'x', sourceX, evtSum.cluster_x.at(maxIdx), evtSum.cluster_x.at(secIdx)
    #print 'y', sourceY, evtSum.cluster_y.at(maxIdx), evtSum.cluster_z.at(secIdx)
    #print 'z', sourceZ, evtSum.cluster_y.at(maxIdx), evtSum.cluster_z.at(secIdx)
    vecMax.SetXYZ(evtSum.cluster_x.at(maxIdx)-sourceX, evtSum.cluster_y.at(maxIdx)-sourceY, evtSum.cluster_z.at(maxIdx)-sourceZ)
    vecSec.SetXYZ(evtSum.cluster_x.at(secIdx)-evtSum.cluster_x.at(maxIdx), evtSum.cluster_y.at(secIdx)-evtSum.cluster_y.at(maxIdx), evtSum.cluster_z.at(secIdx)-evtSum.cluster_z.at(maxIdx))
    
    angleEne = vecMax.Angle(vecSec)
    #print angle
    #remEne = evtSum.energy_mc - evtSum.cluster_energy.at(maxIdx)
    #print angle, evtSum.cluster_energy.at(maxIdx)
    
    diffEne = ROOT.TMath.Abs(evtSum.cluster_energy.at(secIdx) - cF.Eval(angleEne))

    maxIdxTemp = maxIdx
    maxIdx = secIdx
    secIdx = maxIdxTemp
    vecMax.SetXYZ(evtSum.cluster_x.at(maxIdx)-sourceX, evtSum.cluster_y.at(maxIdx)-sourceY, evtSum.cluster_z.at(maxIdx)-sourceZ)
    vecSec.SetXYZ(evtSum.cluster_x.at(secIdx)-evtSum.cluster_x.at(maxIdx), evtSum.cluster_y.at(secIdx)-evtSum.cluster_y.at(maxIdx), evtSum.cluster_z.at(secIdx)-evtSum.cluster_z.at(maxIdx))
    angleDist = vecMax.Angle(vecSec)
    diffDist = ROOT.TMath.Abs(evtSum.cluster_energy.at(secIdx) - cF.Eval(angleDist))
    
    if diffEne < diffDist:
        maxIdxTemp = maxIdx
        maxIdx = secIdx
        secIdx = maxIdxTemp
        angle = angleEne
    else:
        angle = angleDist

    hEvsA.Fill(angle,evtSum.cluster_energy.at(secIdx),1)
    
    res = resolF.Eval(evtSum.cluster_energy.at(secIdx))
    pull = (evtSum.cluster_energy.at(secIdx) - cF.Eval(angle))/(res*res + 30*30)**0.5
    hEpull.Fill(pull)

    if angle > 2.58 and angle < 2.61:
        hEsel.Fill(evtSum.cluster_energy.at(secIdx),1)#evtSum.weight)

myC = ROOT.TCanvas()
myC.Divide(2,1)
myC.cd(1)
hEvsA.Draw()
myC.cd(2)
#hEpull.Draw()
hEsel.Draw()
#myC.cd(3)
#cF.Draw()
myC.cd(1)
cF.Draw('same')

raw_input('end')
