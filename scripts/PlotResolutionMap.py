
import ROOT, settings
import sys, argparse, os, numpy, math
from array import *

ROOT.gSystem.Load('../lib/libEXOEnergy.so')

def MakeResolutionMap(infoRuns):
	nRslices = 6
	rMin = 0.0
	rMax = 160.0
	Rstep = (rMax - rMin)/nRslices

	nZslices = 18
	zMin = -180.0
	zMax = 180.0
	Zstep = (zMax - zMin)/nZslices

	r = math.sqrt(rMax*rMax/nRslices)

	rBins = array('d')
	for i in range(0,nRslices+1):
		rBins.append(math.sqrt(i)*r)

	zBins = array('d')
	for i in range(0,nZslices+1):
		zBins.append(zMin+i*Zstep)

	h2D = ROOT.TH2F('h2D','h2D',nRslices,rBins,nZslices,zBins)
	h2DPeak = ROOT.TH2F('h2DPeak','h2DPeak',nRslices,rBins,nZslices,zBins)
	h1DPeak = ROOT.TH1F('h1DPeak','h1DPeak',nZslices,zBins)

	outFileName = 'outputTest.root'

	histos = LoadData(infoRuns, rBins, zBins)

	for n in range(0, nRslices):
		for m in range(0, nZslices):
			index = n*nZslices + m

			print 'Working on slice: (%.2f,%.2f) (%.2f,%.2f)' % (rBins[n], rBins[n+1], zBins[m], zBins[m+1])

			result = FitData(histos[index])

			resolution = result[1]/result[0]
			peak = result[0]

			peakError = result[2]

			h2D.Fill((rBins[n+1]+rBins[n])/2.0,(zBins[m+1]+zBins[m])/2.0,resolution)
			h2DPeak.Fill((rBins[n+1]+rBins[n])/2.0,(zBins[m+1]+zBins[m])/2.0,peak)

			h2DPeak.SetBinError(h2DPeak.FindBin((rBins[n+1]+rBins[n])/2.0,(zBins[m+1]+zBins[m])/2.0),peakError)

			print '     Resolution: %f' % resolution

			outFile = ROOT.TFile(outFileName,'UPDATE')

			histos[index].Write()

			outFile.Close()

	outFile = ROOT.TFile(outFileName,'UPDATE')

	h2D.Write()
	h2DPeak.Write()

	outFile.Close()

def MakeZBiasPlot(infoRuns):
	nRslices = 1
	rMin = 0.0
	rMax = 160.0

	nZslices = 18
	zMin = -180.0
	zMax = 180.0
	Zstep = (zMax - zMin)/nZslices

	r = math.sqrt(rMax*rMax/nRslices)

	rBins = array('d')
	for i in range(0,nRslices+1):
		rBins.append(math.sqrt(i)*r)

	zBins = array('d')
	for i in range(0,nZslices+1):
		zBins.append(zMin+i*Zstep)

	h1DPeak = ROOT.TH1F('h1DPeak','h1DPeak',nZslices,zBins)

	histos = LoadData(infoRuns, rBins, zBins)

	outFileName = 'outputZ.root'

	for m in range(0, nZslices):
		print 'Working on slice: (%.2f,%.2f) (%.2f,%.2f)' % (rBins[0], rBins[1], zBins[m], zBins[m+1])

		result = FitData(histos[m])

		resolution = result[1]/result[0]
		peak = result[0]

		peakError = result[2]

		h1DPeak.Fill((zBins[m+1]+zBins[m])/2.0,peak)

		h1DPeak.SetBinError(h1DPeak.FindBin((zBins[m+1]+zBins[m])/2.0),peakError)

		print '     Resolution: %f' % resolution

		outFile = ROOT.TFile(outFileName,'UPDATE')

		histos[m].Write()

		outFile.Close()

	outFile = ROOT.TFile(outFileName,'UPDATE')

	h1DPeak.Write()

	outFile.Close()

def MakeAngleResolutionPlot(infoRuns):
	nSlices = 50
	minAngle = 0
	maxAngle = 90
	angleStep = (maxAngle-minAngle)/nSlices

def LoadData(infoRuns, rBins, zBins):
	allWeeks = infoRuns.GetListOf('WeekIndex')
	allRuns = infoRuns.GetListOf('RunNumber')

	histos = []

	print 'Preparing histograms:'
	for i in range(0,len(rBins)-1):
			for j in range(0,len(zBins)-1):
				print '     [(%f,%f) (%f,%f)]' % (rBins[i], rBins[i+1], zBins[j], zBins[j+1])
				histo = ROOT.TH1F('histo (%.2f,%.2f) (%.2f,%.2f)' % (rBins[i], rBins[i+1], zBins[j], zBins[j+1]),'histo (%.2f,%.2f) (%.2f,%.2f)' % (rBins[i], rBins[i+1], zBins[j], zBins[j+1]),220,800,3000)

				histo.SetLineColor(1)
				histo.SetMarkerColor(1)

				histos.append(histo)

	for i in range(allWeeks.size()):
		week = allWeeks.at(i)
		run = allRuns.at(i)
		angle = GetAngleForWeek(week)

		calibPars = GetCalibPars(week)
		if not calibPars:
			print 'Calibration parameters not found for run %s. Skipping this run.' % (run)
			continue

		print 'Adding run %s (week %s): angle = %f, p0 = %f, p1 = %f' % (run,week,angle,calibPars[0],calibPars[1])

		FillDataOfRun(histos, run, angle, rBins, zBins, calibPars[0], calibPars[1])

	return histos

def GetCalibPars(week):
	f = ROOT.TFile(settings.CalibrationOutput+'fit_week_%s_ss.root' % (week),'READ')

	if f.IsZombie():
		return None

	calib = f.Get('calib')

	calibPar = [calib.GetParameter(0),calib.GetParameter(1)]

	return calibPar

def FillDataOfRun(histos, runNumber, angle, rBins, zBins, calibPar0, calibPar1):
	f = ROOT.TFile(settings.SelectionTreeFileName.replace('[RunNumber]',runNumber),'READ')

	entryList = f.Get('EventList_ss')
	treeName = entryList.GetTreeName()
	fileName = entryList.GetFileName()

	f2 = ROOT.TFile(fileName,'READ')
	tree = f2.Get(treeName)
	tree.SetEntryList(entryList)

	for i in range(0,len(rBins)-1):
		for j in range(0,len(zBins)-1):
			index = i*(len(zBins)-1) + j

			print '     [(%f,%f) (%f,%f)]' % (rBins[i], rBins[i+1], zBins[j], zBins[j+1])

			tree.Draw('(cos(%f)*e_charge + sin(%f)*e_scint)*%f+%f' % (angle, angle, calibPar1, calibPar0),'multiplicity < 1.5 && sqrt(cluster_x*cluster_x + cluster_y*cluster_y) < %f && sqrt(cluster_x*cluster_x + cluster_y*cluster_y) >= %f && cluster_z < %f && cluster_z >= %f' % (rBins[i+1], rBins[i], zBins[j+1], zBins[j]),'goff')

			if tree.GetSelectedRows() > 0:
				histos[index].FillN(tree.GetSelectedRows(), tree.GetV1(), numpy.ones(tree.GetSelectedRows()))

	return

def FitData(histo):
	fit = ROOT.TF1('fit','[0]*TMath::Erfc((x-[1])/(TMath::Sqrt(2)*[2]))+[3]*TMath::Gaus(x,[1],[2])',2615*0.93,3000)

	entries = histo.Integral(histo.GetXaxis().FindBin(2615*0.985),histo.GetXaxis().FindBin(2615*1.015))

	fit.SetParameters(entries*0.01,2615,2615*0.03,entries*0.1)

	fit.SetParLimits(1,2550,2680)
	fit.SetParLimits(2,30,200)

	fit.SetLineColor(4)
	fit.SetLineWidth(2)

	histo.Fit('fit','rq')

	resolution = fit.GetParameter(2)/fit.GetParameter(1)

	result = [fit.GetParameter(1),fit.GetParameter(2),fit.GetParError(1),fit.GetParError(2)]

	return result

def GetAngleForWeek(week):
	t = ROOT.TTree()
	t.ReadFile(settings.angleFile)

	t.Draw('angle','week == '+week+' && mult == 1','goff')

	angle = t.GetV1()[0]

	return angle

if __name__ == '__main__':

	settings.init()
	
	infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)
	
	weekStart = 248
	weekEnd = 248
	
	infoRuns.CutDoubleComparison('WeekIndex',weekStart,True)
	infoRuns.CutDoubleComparison('WeekIndex',weekEnd,False)

	infoRuns.CutDefaultRuns()
	
	infoRuns.CutExact('SourceName','Th-228',True)

	MakeResolutionMap(infoRuns)
	#MakeZBiasPlot(infoRuns)

