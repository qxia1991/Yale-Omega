
import ROOT
ROOT.gSystem.Load('libEXOCalibUtilities')
ROOT.gStyle.SetOptStat(0)

mult = 'ms'
week = '092'

fitResultDir = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/results/calibration/2014_Majoron/rotated/from_no_src_activity/'#'2014_05_19/from_no_src_activity/'
fitResultFileName = 'fit_week_%s_%s.root' % (week,mult)
fitResultRuns = range(5327,5337)
fitResultDataHistoNamePat = 'DataEntry_histo_all_fit_week_%s_%s_mcbasedfit_2014v1both_run_%i_%s_data'
fitResultMCHistoNamePat = 'DataEntry_histo_all_fit_week_%s_%s_mcbasedfit_2014v1both_run_%i_%s_smearedmc'
fitResultScaleHistoNamePat = 'DataEntry_histo_all_fit_week_%s_%s_mcbasedfit_2014v1both_run_%i_%s_scale'

rin = ROOT.TFile(fitResultDir+fitResultFileName,'read')
hData = None
hMC = None
hScale = None
for run in fitResultRuns:
    if not hData:
        hData = rin.Get(fitResultDataHistoNamePat % (week,mult,run,mult))
    else:
        hData.Add(rin.Get(fitResultDataHistoNamePat % (week,mult,run,mult)))

    if not hMC:
        hMC = rin.Get(fitResultMCHistoNamePat % (week,mult,run,mult))
    else:
        hMC.Add(rin.Get(fitResultMCHistoNamePat % (week,mult,run,mult)))

    if not hScale:
        hScale = rin.Get(fitResultScaleHistoNamePat % (week,mult,run,mult))
    else:
        hScale.Add(rin.Get(fitResultScaleHistoNamePat % (week,mult,run,mult)))

nBins = 150#200#134#126
lowEnergy = 1000#0#1320#980
upEnergy = 4000#3500
hDataBin = ROOT.TH1D('hDataBin','',nBins,lowEnergy,upEnergy)
hMCBin = ROOT.TH1D('hMCBin','',nBins,lowEnergy,upEnergy)
#hData.Sumw2()
#hData.Scale(100./hData.Integral())
#hMC.Scale(100./hMC.Integral())
for i in range(1,hData.GetNbinsX()+1):
    if hData.FindBin(hDataBin.GetBinCenter(i)) > hData.GetNbinsX():
        continue
    hDataBin.SetBinContent(i,hData.GetBinContent(hData.FindBin(hDataBin.GetBinCenter(i))))
    hMCBin.SetBinContent(i,hMC.GetBinContent(hMC.FindBin(hMCBin.GetBinCenter(i))))
          
hDataBin.Sumw2()
hMCBin.Sumw2()

hDataBin.Scale(100./hDataBin.Integral())
hMCBin.Scale(100./hMCBin.Integral())
#hDataBin.Draw()
#hMCBin.Draw('hist c same')
#raw_input('go')

#hDataBin.Add(hMCBin,-1)
#hDataBin.Divide(hMCBin)
#hDataBin.Draw()
#raw_input('res')

calib = rin.Get('calib')
calib.Print()
mcHisto = ROOT.TH1D('mcHisto','',1000,0,5000)
evtSum = ROOT.EXOEventSummary()
mcFileName = '/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_Majoron/SourceS5_Ra226_fv_162_10_182_%s.root' % (mult)
rmc = ROOT.TFile(mcFileName,'read')
mcTree = rmc.Get('mcTree')
mcTree.SetEstimate(mcTree.GetEntries()+1)
mcTree.Draw('energy_mc','','para goff')
#mcTree.SetBranchAddress('EventSummary',evtSum)
#for i in range(mcTree.GetEntries()):
for i in range(mcTree.GetSelectedRows()):
    if i % 250000 == 0:
        print i, 'of', mcTree.GetSelectedRows()#mcTree.GetEntries()
    #mcTree.GetEntry(i)
    #mcHisto.Fill(calib.GetX(evtSum.energy_mc))
    mcHisto.Fill(calib.GetX(mcTree.GetV1()[i]))
rmc.Close()
mcHisto.Rebin(4)

#mcHisto.Multiply(hScale)
mcHisto.Scale(hMCBin.GetMaximum()/mcHisto.GetMaximum()*(1.,3.)[mult == 'ss'])
mcColor = 2#ROOT.TColor.kBlue-7
mcHisto.SetLineColor(mcColor)
mcHisto.SetLineStyle(2)

xAxis = mcHisto.GetXaxis()
xMin = lowEnergy#1320#980 #xMin = (450,1050)[mult == 'ss']
xMax = upEnergy#4800#3500 #xMax = 3550
xAxis.SetRangeUser(xMin,xMax)
xAxis.SetTitle('Energy Before Calibration (a.u.)')
xAxis.SetTitleSize(0.05)
xAxis.SetTitleOffset(0.90)
xAxis.CenterTitle()
xAxis.SetLabelSize(0.05)

yAxis = mcHisto.GetYaxis()
yAxis.SetTitle('Normalized Counts (%)')
yAxis.SetTitleSize(0.05)
yAxis.SetTitleOffset(0.60)
yAxis.CenterTitle()
yAxis.SetLabelSize(0.05)

hDataBin.SetLineColor(1)
hDataBin.SetLineWidth(2)
hMCBin.SetLineColor(4)
hMCBin.SetLineWidth(2)

#hData.GetXaxis().SetRangeUser(500,3500)

myC = ROOT.TCanvas()

myC.cd()
mcHisto.Draw()
hMCBin.Draw('hist same C')
hDataBin.Draw('same E')

legend = ROOT.TLegend(0.1,0.7,0.48,0.9)
legend.AddEntry(hDataBin,'Data Before Calibration','PEL')
legend.AddEntry(hMCBin,'Fitted MC Simulation','L')
legend.AddEntry(mcHisto,'Initial MC Simulation','L')
legend.SetFillColor(0)
legend.Draw()

myC.Update()

mcAxis = ROOT.TGaxis(ROOT.gPad.GetUxmin(),ROOT.gPad.GetUymax(),ROOT.gPad.GetUxmax(),ROOT.gPad.GetUymax(),calib.Eval(xMin),calib.Eval(xMax),510,'-')
mcAxis.SetLineColor(mcColor)
mcAxis.SetLabelColor(mcColor)
mcAxis.SetLabelFont(xAxis.GetLabelFont())
mcAxis.SetLabelSize(xAxis.GetLabelSize())
mcAxis.SetTitle('Simulated Energy (keV)')
mcAxis.SetTitleFont(xAxis.GetTitleFont())
mcAxis.SetTitleColor(mcColor)
mcAxis.SetTitleSize(xAxis.GetTitleSize())
mcAxis.SetTitleOffset(xAxis.GetTitleOffset())
mcAxis.CenterTitle()

myC.cd()
mcAxis.Draw()
myC.Update()

raw_input('check')
rin.Close()

