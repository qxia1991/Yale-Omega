
import ROOT
import numpy, sys

ROOT.gSystem.Load('libEXOCalibUtilities')
ROOT.gSystem.Load('../lib/libEXOEnergy.so')

multSource = True#False
seedP = int(sys.argv[1])
seedD = int(sys.argv[2])
randP = ROOT.TRandom3(seedP)
randD = ROOT.TRandom3(seedD)
nPar = 3
h = {}
hd = {}

for mult in ['ss','ms']:
    h[mult] = {}
    hd[mult] = {}
    for p in range(nPar):
        h[mult][p] = ROOT.TH1D('%s_p%d'%(mult,p),'',2000,-10,10)
        hd[mult][p] = ROOT.TH1D('%s_dp%d'%(mult,p),'',2000,-10,10)
    fileDir = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_04_14_calib/'
    fileName = 'SourceS5_Th228_fv_162_10_182_%s.root' % (mult)#'SourceS5_Th228_fv_162_10_182_%s.root' % (mult)
    rmc = ROOT.TFile(fileDir+fileName,'read')
    print fileDir+fileName
    tree = rmc.Get('mcTree')
    print tree
    tree.SetEstimate(tree.GetEntries()+1)
    tree.Draw('energy_mc:weight','','para goff')
    nEvents = tree.GetSelectedRows()
    print nEvents
    energies = []
    weights = []
    for i in range(nEvents):
        energies.append(tree.GetV1()[i])
        weights.append(tree.GetV2()[i])
    rmc.Close()
    if multSource:
        fileDir = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_04_14_calib/'
        fileName = 'SourceS5_Co60_fv_162_10_182_%s.root' % (mult)#'SourceS5_Th228_fv_162_10_182_%s.root' % (mult)
        rmc = ROOT.TFile(fileDir+fileName,'read')
        print fileDir+fileName
        tree = rmc.Get('mcTree')
        print tree
        tree.SetEstimate(tree.GetEntries()+1)
        tree.Draw('energy_mc:weight','','para goff')
        nEvents = tree.GetSelectedRows()
        print nEvents
        for i in range(nEvents):
            energies.append(tree.GetV1()[i])
            weights.append(tree.GetV2()[i])
        rmc.Close()

        fileDir = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_04_14_calib/'
        fileName = 'SourceS2_Ra226_fv_162_10_182_%s.root' % (mult)#'SourceS5_Th228_fv_162_10_182_%s.root' % (mult)
        rmc = ROOT.TFile(fileDir+fileName,'read')
        print fileDir+fileName
        tree = rmc.Get('mcTree')
        print tree
        tree.SetEstimate(tree.GetEntries()+1)
        tree.Draw('energy_mc:weight','','para goff')
        nEvents = tree.GetSelectedRows()
        print nEvents
        for i in range(nEvents):
            energies.append(tree.GetV1()[i])
            weights.append(tree.GetV2()[i])
        rmc.Close()

    for p in range(1):
        p0 = randP.Gaus(20,20*0.1)#randP.Gaus(0.5,0.5*0.1)
        p1 = randP.Gaus(0.7,0.7*0.1)#randP.Gaus(20,20*0.1)
        p2 = randP.Gaus(2.5*1e-7,0.1*2.5*1e-7)#randP.Gaus(0.01,0.01*0.1) #0.005
        ps = [p0,p1,p2]
        print p, ps
        smearer = ROOT.EXOEnergyMCBasedFit1D()
        smearer.SetVerboseLevel(-1)
        smearer.AddMC('MC',0,0,0.,0.,0.,numpy.array(energies),numpy.array(weights),len(energies))
        smearer.AddData('data','MC',0,0,0.,0.,0.,0,numpy.array([0]),len([0]))
        smearer.BinMCEnergy(1)
        histos = ROOT.std.vector('TString')()
        histos.push_back(ROOT.TString('data'))
        smearer.SetDataHisto('FitHisto',histos,50000,0,5000)
        calib = ROOT.TF1('calib','x',0,5000)
        resol = ROOT.TF1('resol','sqrt([0]*[0]*x + [1]*[1] + [2]*[2]*x*x)',0,5000)
        resol.FixParameter(0,0.5)#p0)
        resol.FixParameter(1,20)#p1)
        resol.FixParameter(2,0.005)#p2)
        smearer.SetFunction('calib',calib)
        smearer.SetFunction('resol',resol)
        smearer.ApplyFittedCalibration(50000,0,5000)

        smearedMC = smearer.GetHisto('MC','MC','')

        for j in range(2):
            print j
            ROOT.EXOEnergyMCBasedFitBase.fFCN = 0
            fakeData = []
            for b in range(1,smearedMC.GetNbinsX()+1):
                fakeEnergy = smearedMC.GetBinCenter(b)
                mcExpected = smearedMC.GetBinContent(b)/10.
                fakeObserved = randD.Poisson(mcExpected)
                fakeEnergy = (-p1 + (p1*p1 - 4*p2*(p0-fakeEnergy))**0.5)/(2*p2)#
                #fakeEnergy = (fakeEnergy-p0)/p1#(-p1 + (p1*p1 - 4*p2*(p0-fakeEnergy))**0.5)/(2*p2)
                if fakeEnergy > 0 and fakeObserved > 0:
                    for n in range(fakeObserved):
                        fakeData.append(fakeEnergy)
            
            fitter = ROOT.EXOEnergyMCBasedFit1D()
            fitter.SetVerboseLevel(-1)
            fitter.AddMC('fitMC',0,0,0.,0.,0.,numpy.array(energies),numpy.array(weights),len(energies))
            fitter.AddData('fitData','fitMC',0,0,0.,0.,0.,0,numpy.array(fakeData),len(fakeData))
            fitter.BinMCEnergy(1)
            histo = ROOT.std.vector('TString')()
            histo.push_back(ROOT.TString('fitData'))
            fitter.SetDataHisto('fitFitHisto',histo,1000,0,5000)#800,1000,5000)#1000,0,5000)
            calib = ROOT.TF1('calib','[0]+[1]*x+[2]*x*x',0,5000)
            calib.SetParameters(numpy.array([20,0.7,2.5*1e-7]))
            calib.SetParErrors(numpy.array([2,0.07,1e-8]))
            resol = ROOT.TF1('resol','sqrt([0]*[0]*x + [1]*[1] + [2]*[2]*x*x)',0,5000)
            resol.FixParameter(0,0.5)#p0)
            resol.FixParameter(1,20)#p1)
            resol.FixParameter(2,0.005)#p2)
            #resol.SetParameters(numpy.array([0.1,10,0.001]))
            #resol.SetParErrors(numpy.array([0.1,10,0.001]))
            fitter.SetFunction('calib',calib)
            fitter.SetFunction('resol',resol)
            fitter.ExecuteFit(1,'MIGRAD',1.)
            fitter.SaveHistosIn('test_%s.root'%(mult),'recreate')
            fitter.GetFitter().ExecuteCommand('MINOS',numpy.array(([10000,0,1],[10000,0,1,2])[nPar == 3]),nPar+1)
            eplus = ROOT.Double()
            eminus = ROOT.Double()
            eparab = ROOT.Double()
            globcc = ROOT.Double()
            for p in range(nPar):
                fitter.GetFitter().GetErrors(p,eplus,eminus,eparab,globcc)
                value = fitter.GetFitter().GetParameter(p)
                error = (-eminus,eplus)[value > ps[p]]
                if error > 0:
                    h[mult][p].Fill((value-ps[p])/error)
                diff = (value-ps[p])/ps[p]
                hd[mult][p].Fill(diff)
                
                print value, ps[p], float(eplus), float(eminus), float(eparab), float(globcc)
            del fitter

        del smearer
        #smearedMC.Draw('histo C')
            #raw_input('hello')


rout = ROOT.TFile(sys.argv[3],'recreate')
for mult in ['ss','ms']:
    for p in range(nPar):
        rout.cd()
        h[mult][p].Write()
        hd[mult][p].Write()
rout.Close()
