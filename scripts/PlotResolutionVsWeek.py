
import ROOT, settings
import os, fnmatch, re

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

settings.init()

gr1 = ROOT.TGraph()
gr2 = ROOT.TGraph()
gr3 = ROOT.TGraph()
gr4 = ROOT.TGraph()

i = 0

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

files = os.listdir(settings.CalibrationOutput)

fit = ROOT.TF1("fit","[0]*TMath::Erfc((x-[1])/(TMath::Sqrt(2)*[2]))+[3]*TMath::Gaus(x,[1],[2])",fitRangeMin,fitRangeMax)

fit.SetLineWidth(1)

for file in sorted(files):
	if fnmatch.fnmatch(file, 'fit_week_*_ss.root'):
		f = ROOT.TFile(settings.CalibrationOutput+file,'READ')
		matchObj = re.match('fit_week_([0-9]{3})_ss.root',file)
		week = matchObj.group(1)

		if float(week) < settings.minWeek:
			continue
		if float(week) > settings.maxWeek:
			continue

		calib = f.Get('calib')
		resol = f.Get('resol')
		resolution1 = resol.Eval(2615.0)/2615.0*100.0
		resolution2 = resol.Eval(2458.0)/2458.0*100.0
		gr1.SetPoint(i,float(week),resolution1)
		gr2.SetPoint(i,float(week),resolution2)

		if printParameters:
			print "%s 1 %f %f %f %f" % (week, calib.GetParameter(0), calib.GetParameter(1), resol.GetParameter(0), resol.GetParameter(1))
		else:
			print "file %s, week %s, resolution = %f" % (file, week, resolution1)

		h = f.Get('FitHistoEntry_histo_all_fit_week_%s_ss_data' % week)

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

		fOut.Close()

		i = i+1

h_empty = ROOT.TH2F("h_empty","h_empty",100,settings.minWeek-5,settings.maxWeek+5,100,0,8)

h_empty.GetXaxis().SetTitle("week")
h_empty.GetYaxis().SetTitle("resolution #sigma/E (%)")

h_empty.GetXaxis().CenterTitle()
h_empty.GetYaxis().CenterTitle()

gr1.SetMarkerColor(633)
gr1.SetMarkerStyle(22)
gr1.SetMarkerSize(0.8)

gr2.SetMarkerColor(601)
gr2.SetMarkerStyle(21)
gr2.SetMarkerSize(0.8)

gr3.SetMarkerColor(620)
gr3.SetMarkerStyle(23)
gr3.SetMarkerSize(0.8)

gr4.SetMarkerColor(633)
gr4.SetMarkerStyle(20)
gr4.SetMarkerSize(0.8)

c1 = ROOT.TCanvas()
h_empty.Draw()
gr1.Draw("Psame")
gr2.Draw("Psame")
gr3.Draw("Psame")

c3 = ROOT.TCanvas()
gr4.Draw("AP")

fOut = ROOT.TFile("resolution_"+settings.calibrationChannel+".root","UPDATE")

c1.Write()
c3.Write()

fOut.Close()
