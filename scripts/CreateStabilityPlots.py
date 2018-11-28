
import ROOT, settings
import os, fnmatch, re

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

settings.init()

gr1 = ROOT.TGraph()
gr2 = ROOT.TGraph()

gr3 = ROOT.TGraph()
gr4 = ROOT.TGraph()
gr5 = ROOT.TGraph()
gr6 = ROOT.TGraph()

i = 0

printParameters = False

files = os.listdir(settings.CalibrationOutput)

for inputFile in sorted(files):
	if fnmatch.fnmatch(inputFile, 'fit_week_*_ss.root'):
		f = ROOT.TFile(settings.CalibrationOutput+inputFile,'READ')
		matchObj = re.match('fit_week_([0-9]{3})_ss.root',inputFile)
		week = matchObj.group(1)
		calib = f.Get('calib')
		resol = f.Get('resol')
		resolution1 = resol.Eval(2615.0)/2615.0*100.0
		resolution2 = resol.Eval(2458.0)/2458.0*100.0
		gr1.SetPoint(i,float(week),resolution1)
		gr2.SetPoint(i,float(week),resolution2)

		gr3.SetPoint(i,float(week),calib.GetParameter(0))
		gr4.SetPoint(i,float(week),calib.GetParameter(1))
		gr5.SetPoint(i,float(week),resol.GetParameter(0))
		gr6.SetPoint(i,float(week),resol.GetParameter(1))

		if printParameters:
			print "%s 1 %f %f %f %f" % (week, calib.GetParameter(0), calib.GetParameter(1), resol.GetParameter(0), resol.GetParameter(1))
		else:
			print "file %s, week %s, resolution = %f" % (inputFile, week, resolution1)

		i = i+1

		f.Close()

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

gr3.SetMarkerStyle(20)
gr3.SetMarkerSize(0.8)

gr4.SetMarkerStyle(20)
gr4.SetMarkerSize(0.8)

gr5.SetMarkerStyle(20)
gr5.SetMarkerSize(0.8)

gr6.SetMarkerStyle(20)
gr6.SetMarkerSize(0.8)

c1 = ROOT.TCanvas("resol","resol")
h_empty.Draw()
gr1.Draw("Psame")
gr2.Draw("Psame")

c2 = ROOT.TCanvas("calib_p0","calib_p0")
gr3.Draw("AP")

c3 = ROOT.TCanvas("calib_p1","calib_p0")
gr4.Draw("AP")

c4 = ROOT.TCanvas("resol_p0","resol_p0")
gr5.Draw("AP")

c5 = ROOT.TCanvas("resol_p1","resol_p1")
gr6.Draw("AP")

f2 = ROOT.TFile("stabilityPlots_"+settings.calibrationChannel+".root","RECREATE")

c1.Write()
c2.Write()
c3.Write()
c4.Write()
c5.Write()

f2.Close()

