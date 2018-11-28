
import ROOT, settings
import os, fnmatch, re

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

ROOT.gStyle.SetOptTitle(0)
ROOT.gStyle.SetOptStat(0)

basedir = '/nfs/slac/g/exo_data4/users/Energy/data/results/calibration/'
directoryPhase1 = basedir+'2017_Phase1_v2/rotated/fv_162_10_182/'
directoryPhase2_8kV = basedir+'2016_110116/rotated/fv_162_10_182/'
directoryPhase2 = basedir+'2017_Phase2_050217/FullRange/rotated/fv_162_10_182/'
directoryPhase1NonDenoised = basedir+'2015_Cosmogenics_cp/rotated/fv_162_5_182/'
directoryPhase1dNonDenoised = basedir+'2017_Phase1_NonDenoised_071217/rotated/fv_162_10_182/'

weekMinPhase1 = 10
weekMaxPhase1 = 125
weekMinPhase2 = 229
weekMaxPhase2 = 292

plotDateLabels = True

excludedWeeks = []
excludedWeeks.append(15) # bad fit
excludedWeeks.append(16) # bad fit
excludedWeeks.append(18) # only run with bad purity
excludedWeeks.append(51) # bad purity
excludedWeeks.append(52) # bad purity
excludedWeeks.append(57) # marked as suspect
excludedWeeks.append(76) # no source runs
excludedWeeks.append(112) # bad fit
excludedWeeks.append(114) # bad purity
excludedWeeks.append(115) # bad purity
excludedWeeks.append(117) # bad purity
excludedWeeks.append(240) # bad fit
excludedWeeks.append(245) # run with only Ra

gr1 = ROOT.TGraph()
gr2 = ROOT.TGraph()
gr3 = ROOT.TGraph()
gr4 = ROOT.TGraph()

i = 0
k = 0

printParameters = False
isCalibrated = False

if isCalibrated:
	fitRangeMin = 2450
	fitRangeMax = 2800
	fitMean = 2615
else:
	fitRangeMin = 3900
	fitRangeMax = 5000
	fitMean = 4100

filesPhase1 = os.listdir(directoryPhase1)
filesPhase2_8kV = os.listdir(directoryPhase2_8kV)
filesPhase2 = os.listdir(directoryPhase2)
filesPhase1NonDenoised = os.listdir(directoryPhase1NonDenoised)
filesPhase1dNonDenoised = os.listdir(directoryPhase1dNonDenoised)

files = []
for file in filesPhase1:
	files.append(directoryPhase1+file)
for file in filesPhase2_8kV:
	matchObj = re.match('fit_week_([0-9]{3}).*',file)
	if not matchObj:
		continue
        week = matchObj.group(1)
	if float(week) >= 244:
		continue
	files.append(directoryPhase2_8kV+file)
for file in filesPhase2:
	files.append(directoryPhase2+file)
for file in filesPhase1NonDenoised:
	files.append(directoryPhase1NonDenoised+file)
for file in filesPhase1dNonDenoised:
	files.append(directoryPhase1dNonDenoised+file)

fit = ROOT.TF1("fit","[0]*TMath::Erfc((x-[1])/(TMath::Sqrt(2)*[2]))+[3]*TMath::Gaus(x,[1],[2])",fitRangeMin,fitRangeMax)

fit.SetLineWidth(1)

for file in files:
	if fnmatch.fnmatch(file, '*/fit_week_*_ss.root'):
		print file
		f = ROOT.TFile(file,'READ')
		matchObj = re.match('.*/fit_week_([0-9]{3})_ss.root',file)
		week = matchObj.group(1)

		if float(week) < weekMinPhase1:
			continue
		if float(week) > weekMaxPhase1 and float(week) < weekMinPhase2:
			continue
		if float(week) > weekMaxPhase2:
			continue
		if float(week) in excludedWeeks:
			continue
		
		calib = f.Get('calib')
		resol = f.Get('resol')
		resolution1 = resol.Eval(2615.0)/2615.0*100.0
		resolution2 = resol.Eval(2458.0)/2458.0*100.0

		if float(week) >= weekMinPhase1 and float(week) <= weekMaxPhase1 and i > weekMaxPhase1:
			gr3.SetPoint(k,float(week),resolution1)
			k = k+1
		elif float(week) >= weekMinPhase2:
			gr3.SetPoint(i,float(week),resolution1)
                        gr2.SetPoint(i,float(week),resolution2)
		else:
			gr1.SetPoint(i,float(week),resolution1)
			gr2.SetPoint(i,float(week),resolution2)

		print "file %s, week %s, resolution = %f" % (file, week, resolution1)

		"""h = f.Get('FitHistoEntry_histo_all_fit_week_%s_ss_data' % week)

		fit.SetParameters(100,fitMean,100,1000)
		
		h.Fit('fit','rn')

		resolGausErrf = fit.GetParameter(2)/fit.GetParameter(1)*100

		gr3.SetPoint(i,float(week),resolGausErrf)
		gr4.SetPoint(i,float(week),fit.GetParameter(1)/2615.0)

		fOut = ROOT.TFile("resolution_"+settings.calibrationChannel+".root","UPDATE")

		c2 = ROOT.TCanvas('c_week_%s' % week,'c_week_%s' % week)
		h.Draw()
		fit.Draw("same")

		c2.Write()

		fOut.Close()"""

		i = i+1

l1 = ROOT.TLine(233,1,233,2.4)
    
l1.SetLineWidth(2)
l1.SetLineColor(601)
l1.SetLineStyle(2)

l2 = ROOT.TLine(243.5,1,243.5,2.4)

l2.SetLineWidth(2)
l2.SetLineColor(417)
l2.SetLineStyle(2)

l = ROOT.TLegend(0.13,0.72,0.85,0.87)

l.SetNColumns(2)
l.AddEntry(gr1,"Tl-208, denoised","p")
l.AddEntry(gr3,"Tl-208, non-denoised","p")
l.AddEntry(gr2,"Resolution at Q","p")
l.AddEntry(l1,"Electronics upgrade","l")
l.AddEntry(l2,"Cathode bias -12kV","l")

l.SetFillStyle(1001)
l.SetFillColor(0)

dateLabels = []
dateLabels.append(ROOT.TText(1,0.92,'2011-09-18')) # week 1
dateLabels.append(ROOT.TText(40,0.92,'2012-06-17')) # week 40
dateLabels.append(ROOT.TText(80,0.92,'2013-03-24')) # week 80
dateLabels.append(ROOT.TText(120,0.92,'2013-12-29')) # week 120
dateLabels.append(ROOT.TText(240,0.92,'2016-04-17')) # week 240
dateLabels.append(ROOT.TText(280,0.92,'2017-01-22')) # week 280

text2 = ROOT.TText(65,2.48,"Phase I")
text3 = ROOT.TText(263,2.48,"Phase II")

text2.SetTextAlign(21)
text2.SetTextFont(40)
text2.SetTextSize(0.06)

text3.SetTextAlign(21)
text3.SetTextFont(40)
text3.SetTextSize(0.08)

gr1.SetMarkerColor(417)
gr1.SetMarkerStyle(22)
gr1.SetMarkerSize(1.0)

gr2.SetMarkerColor(601)
gr2.SetMarkerStyle(21)
gr2.SetMarkerSize(0.8)

gr3.SetMarkerColor(633)
gr3.SetMarkerStyle(22)
gr3.SetMarkerSize(1.0)

gr4.SetMarkerColor(632)
gr4.SetMarkerStyle(20)
gr4.SetMarkerSize(0.5)

h_empty1 = ROOT.TH2F("h_empty1","h_empty1",100,0,135,100,1,2.4)
h_empty2 = ROOT.TH2F("h_empty2","h_empty2",100,225.5,300,100,1,2.4)

h_empty1.GetYaxis().SetTitle("resolution #sigma/E (%)")
h_empty1.GetYaxis().CenterTitle()

h_empty1.GetXaxis().SetTitle("week")
h_empty1.GetXaxis().CenterTitle()

h_empty1.GetYaxis().SetTitleSize(0.05)
h_empty1.GetYaxis().SetLabelSize(0.05)

h_empty2.GetYaxis().SetLabelSize(0)
h_empty2.GetYaxis().SetTickLength(0)

h_empty2.GetXaxis().SetNdivisions(505)
h_empty2.GetXaxis().SetLabelSize(0.05)
h_empty2.GetXaxis().SetLabelOffset(-0.02)

c1 = ROOT.TCanvas("c1","c1",1000,500)

pad1 = ROOT.TPad("pad1","pad1",0.005,0.005,0.64,0.995)
pad2 = ROOT.TPad("pad2","pad2",0.645,0.005,0.995,0.995)

pad1.SetMargin(0.1,0.01,0.1,0.1)

pad2.SetMargin(0.01,0.1,0.1,0.1)

pad1.Draw()
pad2.Draw()

pad1.SetGridy()
pad1.SetGridx()

pad2.SetGridy()
pad2.SetGridx()

pad1.cd()
h_empty1.Draw()
gr1.Draw("Psame")
gr2.Draw("Psame")
gr3.Draw("Psame")
l.Draw("same")
text2.Draw("same")
if plotDateLabels:
	for i, dateLabel in enumerate(dateLabels):
		if i > 3:
			continue
		dateLabel.SetTextAlign(21)
		dateLabel.SetTextFont(40)
		dateLabel.SetTextSize(0.05)
		dateLabel.Draw("same")
	h_empty1.GetXaxis().SetLabelOffset(100)
	h_empty1.GetXaxis().SetTitleOffset(100)

pad2.cd()
h_empty2.Draw()
gr1.Draw("Psame")
gr2.Draw("Psame")
gr3.Draw("Psame")
l1.Draw("same")
l2.Draw("same")
text3.Draw("same")
if plotDateLabels:
	for i, dateLabel in enumerate(dateLabels):
		if i < 4:
			continue
		dateLabel.SetTextAlign(21)
		dateLabel.SetTextFont(40)
		dateLabel.SetTextSize(0.07)
		dateLabel.Draw("same")
	h_empty2.GetXaxis().SetLabelOffset(100)

c1.SaveAs("ResolutionVsWeekPolished.root")
c1.SaveAs("ResolutionVsWeekPolished.pdf")
c1.SaveAs("ResolutionVsWeekPolished.png")

