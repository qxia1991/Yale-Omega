
import ROOT
import copy, numpy
import sys

ROOT.gSystem.Load('../lib/libEXOEnergy.so')
ROOT.gSystem.Load('libEXOCalibUtilities')

fv_wires,fv_min_z,fv_max_z = 162, 5, 182
cutName = 'fv_%i_%i_%i' % (fv_wires,fv_min_z,fv_max_z)

allSourcePos = ['5']#,'11','2','8']
onlyOldS5 = False
allSourceNames = ['Th-228']#,'Co-60','Ra-226']
calibType = 'Cosmo'#'BugFixRN'#'MC' #'Gaus'
resFlavor = {'Cosmo':'2015-v2-average-linear','BugFixRN':'2014-v3-weekly','MC':'2014-v2-weekly','Gaus':'2014-v1-weekly'}
resTable = {'Cosmo':'energy-mcbased-fit','BugFixRN':'energy-mcbased-fit','MC':'energy-mcbased-fit','Gaus':'energy-resolution'}

useTEntryList = True

mcType = '2015_Cosmogenics'#'Exotica'#'RealNoise'#'RealNoiseBugFix'#'WhiteNoise'#'RealNoiseBugFix'
mcDir = {}
mcDir['RealNoiseBugFix'] = '2014_Majoron'#'RealNoiseBugFix'
mcDir['WhiteNoiseBugFix'] = '2014_Majoron/WhiteNoise'#'WhiteNoiseBugFix'
mcDir['pcdSize/0.1mm'] = 'pcdSize/0.1mm'
mcDir['pcdSize/0.3mm'] = 'pcdSize/0.3mm'
mcDir['RealNoiseBugFixPre'] = 'RealNoiseBugFix'
mcDir['Exotica'] = '2014_Exotica'
mcDir['2015_Cosmogenics'] = '2015_Cosmogenics'

fitOn = False
lowEnergyFit = 980.
upEnergyFit = 2000.
nWeeksCombo = 1#int(sys.argv[1])

outFileName = 'srcAgreePlots_calib%s_mc%s_newZdb_Weekly_Ene980_ThS5Only_newSD_%dweek.root'%(calibType,mcType,nWeeksCombo)

infoRuns = ROOT.EXOSourceRunsPolishedInfo('../data/UpdatedLivetimeSourceRunsInfo.txt')

#Cs at S2, no simulation...
infoRuns.CutExact('RunNumber','2410',False)
infoRuns.CutExact('RunNumber','2450',False)
#Processing problem, should be fixed
infoRuns.CutExact('RunNumber','5822',False)

infoRuns.CutExact('SourcePositionS','0',False)
infoRuns.CutExact('SourcePositionS','17',False)
infoRuns.CutExact('ManualFlag','NONE')
infoRuns.CutExact('Quality','BAD',False)
infoRuns.AddExactCondition('SourceName','Cs-137',True)
infoRuns.AddExactCondition('SourceName','Ra-226',True)
infoRuns.CutDoubleComparison('Purity',2.0,True)
infoRuns.AddExactCondition('SourceName','Co-60',True)
infoRuns.CutDoubleComparison('Purity',9.9,False)
infoRuns.ClearExceptRuns()

infoRuns.CutExact('TriggerPrescale','0')
infoRuns.CutDoubleComparison('RunNumber',5600,False)

#infoRuns.CutExact('RunNumber','3717')

totAbsRes = 0.
totResErr = 0.
outFile = ROOT.TFile(outFileName,'recreate')
#if calibType == 'MC':
#    resol = ROOT.EXOEnergyResol.GetInstanceForFlavor('2014-v2-weekly','energy-mcbased-fit')
#elif calibType == 'Gaus':
#    resol = ROOT.EXOEnergyResol.GetInstanceForFlavor('2014-v1-weekly','energy-resolution')
resol = ROOT.EXOEnergyResol.GetInstanceForFlavor(resFlavor[calibType],resTable[calibType])

for sourceName in allSourceNames:#['Th-228']:#,'Co-60','Ra-226']:
    sourceRuns = copy.copy(infoRuns)
    sourceRuns.CutExact('SourceName',sourceName)

    # only first Ra-226 campaign
    if sourceName == 'Ra-226':
        sourceRuns.CutDoubleComparison('RunNumber',5337,False)

    for sourcePosS in allSourcePos:#['5']:#,'11','2','8']:
        sourcePosRuns = copy.copy(sourceRuns)

        sourcePosRuns.CutExact('SourcePositionS',sourcePosS)
        
        sourcePosRuns.SetMetadata('DataMultiplicityWildCard','[MULTIPLICITY]')

        if calibType == 'MC':
            # calib MC
            sourcePosRuns.SetMetadata('DataTreeFileName','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_06_03/no_src_activity/mcbasedfit_2014v2both_run_[RunNumber]_tree.root')
            sourcePosRuns.SetMetadata('DataSelectionFileName','/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/WIPP/selection/2014_06_03/from_no_src_activity/mcbasedfit_2014v2both_run_[RunNumber]_tree_%s_[MULTIPLICITY].root'%(cutName))
        elif calibType == 'Gaus':
            # gauss calib
            sourcePosRuns.SetMetadata('DataTreeFileName','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_06_13/no_src_activity/ljcalib_2014v1_20130nudenoised_2014v1_run_run_[RunNumber]_tree.root')
            sourcePosRuns.SetMetadata('DataSelectionFileName','/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/WIPP/selection/2014_06_13/from_no_src_activity/ljcalib_2014v1_20130nudenoised_2014v1_run_[RunNumber]_tree_%s_[MULTIPLICITY].root'%(cutName))
        elif calibType == 'BugFixRN':
            # calib MC w/ real noise bug fix MC
            sourcePosRuns.SetMetadata('DataTreeFileName','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_Majoron/no_src_activity/mcbasedfit_2014v3both_run_[RunNumber]_tree.root')
            sourcePosRuns.SetMetadata('DataSelectionFileName','/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/WIPP/selection/2014_Majoron/from_no_src_activity/mcbasedfit_2014v3both_run_[RunNumber]_tree_%s_[MULTIPLICITY].root'%(cutName))            
        elif calibType == 'Cosmo':
            # calib MC-based for cosmogenics analysis: non-denoised, cut, etc.
            sourcePosRuns.SetMetadata('DataSelectionFileName','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/selection/2015_Cosmogenics/AfterCalibration/%s/cosmo_run_[RunNumber]_tree.root'%(cutName))            
            

        if mcType == 'RealNoise':
            # real noise
            sourcePosRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/2014_04_10/') 
            sourcePosRuns.SetMCSelectionFileNames(cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_05_19/','[MULTIPLICITY]')
        elif mcType == 'WhiteNoise':
            # white noise
            sourcePosRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/WhiteNoise/') 
            sourcePosRuns.SetMCSelectionFileNames(cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/WhiteNoise/','[MULTIPLICITY]')
        elif mcType == 'Exotica':
            sourcePosRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/%s/'%(mcDir[mcType])) 
            sourcePosRuns.SetMCSelectionFileNames('default_'+cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/%s/'%(mcDir[mcType]),'[MULTIPLICITY]')            
        elif mcType == '2015_Cosmogenics':
            sourcePosRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/%s/'%(mcDir[mcType])) 
            sourcePosRuns.SetMCSelectionFileNames('default_'+cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/%s/%s/'%(mcDir[mcType],cutName),'[MULTIPLICITY]')            
        else:
            sourcePosRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/%s/'%(mcDir[mcType])) 
            sourcePosRuns.SetMCSelectionFileNames(cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/%s/'%(mcDir[mcType]),'[MULTIPLICITY]')
        
        if onlyOldS5:
            sourcePosRuns.CutExact('MCId','SourceS5_Th228_[MULTIPLICITY]')
        mcFileNames = sourcePosRuns.GetSetOf('MCSelectionFileName')

        for m, mult in enumerate(['ss','ms']):#
            m = m + 1

            plotId = '%s_%s_%s' % (sourceName,sourcePosS,mult)
            nBins = 180#201
            lowEnergy = 980#686
            upEnergy = 3500
            dataHisto = ROOT.TH1D(plotId+'_data','',nBins,lowEnergy,upEnergy)
            mcHisto = ROOT.TH1D(plotId+'_mc','',nBins,lowEnergy,upEnergy)
            resHisto = ROOT.TH1D(plotId+'_res','',nBins,lowEnergy,upEnergy)
            ratioHisto = ROOT.TH1D(plotId+'_ratio','',nBins,lowEnergy,upEnergy)
            ratioXeneHisto = ROOT.TH2D(plotId+'_ratiovsene','',nBins,lowEnergy,upEnergy,2000,0,2)

            nBinsSD = 20#80
            lowSD = 0
            upSD = 200
            sdDataHisto = ROOT.TH1D(plotId+'_data_sd','',nBinsSD,lowSD,upSD)
            sdMCHisto = ROOT.TH1D(plotId+'_mc_sd','',nBinsSD,lowSD,upSD)
            resSDHisto = ROOT.TH1D(plotId+'_res_sd','',nBinsSD,lowSD,upSD)

            resSDGraph = ROOT.TGraphErrors()
            resSDGraph.SetName(plotId+'_res_sd_gr')

            polfitHisto0 = ROOT.TH1D(plotId+'_p0','',200,0,2)
            polfitHisto1 = ROOT.TH1D(plotId+'_p1','',500,-1000,1000)
            polfitGraph0 = ROOT.TGraphErrors()
            polfitGraph0.SetName(plotId+'_gr_p0')
            polfitGraph1 = ROOT.TGraphErrors()
            polfitGraph1.SetName(plotId+'_gr_p1')

            polfitGraph0week = ROOT.TGraphErrors()
            polfitGraph0week.SetName(plotId+'_gr_p0_week')
            polfitGraph1week = ROOT.TGraphErrors()
            polfitGraph1week.SetName(plotId+'_gr_p1_week')

            polfitHisto0week = ROOT.TH1D(plotId+'_p0_week','',200,0,2)
            polfitHisto1week = ROOT.TH1D(plotId+'_p1_week','',500,-1000,1000)
            

            print 'Working on', plotId

            weekSet = sourcePosRuns.GetSetOf('WeekIndex')
            weeklyPolFit = {}
            for week in weekSet:
                week = (int(week)+1)//nWeeksCombo
                if not week in weeklyPolFit:
                    weeklyPolFit[week] = {}
                    weeklyPolFit[week]['MC'] = ROOT.TH1D('week_%s_%s_mc'%(week,plotId),'',nBins,lowEnergy,upEnergy)
                    weeklyPolFit[week]['data'] = ROOT.TH1D('week_%s_%s_data'%(week,plotId),'',nBins,lowEnergy,upEnergy)
                

            for mcFileName in mcFileNames:
                selectRuns = copy.copy(sourcePosRuns)
                selectRuns.CutExact('MCSelectionFileName',mcFileName)
                
                mcFileName = mcFileName.replace('[MULTIPLICITY]',mult)
                print id, sourceName, sourcePosS, mult, m, mcFileName

                mcTreeFile = None
                if useTEntryList:
                    mcFile = ROOT.TFile(mcFileName,'read')
                    mcList = mcFile.Get('EventList_%s'%(mult))
                    mcTreeName = mcList.GetTreeName()
                    mcTreeFileName = mcList.GetFileName()
                    #mcTree = ROOT.TChain(mcTreeName)
                    #mcTree.Add(mcTreeFileName)
                    mcTreeFile = ROOT.TFile(mcTreeFileName,'read')
                    mcTree = mcTreeFile.Get(mcTreeName)
                    mcTree.SetEntryList(mcList)
                else:
                    mcFile = ROOT.TFile(mcFileName,'read')
                    mcTree = mcFile.Get('mcTree')
                mcTree.SetEstimate(mcTree.GetEntries()+1)
                mcTree.Draw('energy_mc:weight:standoff_distance','','para goff')
                #raw_input('entries %d' % (mcTree.GetSelectedRows()))
                #mcSmeared = resol.SmearedMC('Rotated',mcTree.GetV1(),mcTree.GetV2(),mcTree.GetSelectedRows(),m,nBins,lowEnergy,upEnergy,1346773897,0)
                #mcSmeared.Draw('hist C')
                #mcSmeared.Sumw2()
                originalMCweights = []
                for i in range(mcTree.GetSelectedRows()):
                    originalMCweights.append(mcTree.GetV2()[i])

                dataFileNames = selectRuns.GetListOf('DataSelectionFileName')
                dataRunNumbers = selectRuns.GetListOf('RunNumber')
                startTimes = selectRuns.GetListOf('StartTimeSec')
                weekLivetimes = selectRuns.GetListOf('WeekLivetime')
                weekIndices = selectRuns.GetListOf('WeekIndex')                

                for dataFileName,dataRunNumber,startTime,weekLivetime,weekIndex in zip(dataFileNames,dataRunNumbers,startTimes,weekLivetimes,weekIndices):
                    dataFileName = dataFileName.replace('[MULTIPLICITY]',mult)
                    print dataFileName, dataRunNumber
                    dataTreeFile = None
                    if useTEntryList:
                        dataFile = ROOT.TFile(dataFileName,'read')
                        dataList = dataFile.Get('EventList_%s'%(mult))
                        dataTreeName = dataList.GetTreeName()
                        dataTreeFileName = dataList.GetFileName()
                        #dataTree = ROOT.TChain(dataTreeName)
                        #dataTree.Add(dataTreeFileName)
                        dataTreeFile = ROOT.TFile(dataTreeFileName,'read')
                        dataTree = dataTreeFile.Get(dataTreeName)
                        dataTree.SetEntryList(dataList)
                    else:
                        dataFile = ROOT.TFile(dataFileName,'read')
                        dataTree = dataFile.Get('dataTree')

                    dataTree.SetEstimate(dataTree.GetEntries()+1)
                    dataTree.Draw('energy_%s:standoff_distance'%(mult),'%.2f < energy_%s && energy_%s < %.2f'%(lowEnergy,mult,mult,upEnergy),'para goff')
                    
                    dataTemp = ROOT.TH1D('run_%s_%s_data'%(dataRunNumber,plotId),'',nBins,lowEnergy,upEnergy)
                    dataTemp.FillN(dataTree.GetSelectedRows(),dataTree.GetV1(),numpy.ones(dataTree.GetSelectedRows()))
                    sdDataTemp = ROOT.TH1D('run_%s_%s_data_sd'%(dataRunNumber,plotId),'',nBinsSD,lowSD,upSD)
                    sdDataTemp.FillN(dataTree.GetSelectedRows(),dataTree.GetV2(),numpy.ones(dataTree.GetSelectedRows()))

                    mcSmeared = resol.SmearedMC('Rotated',mcTree.GetV1(),mcTree.GetV2(),mcTree.GetSelectedRows(),m,nBins,lowEnergy,upEnergy,int(startTime),0)
                    mcSmeared.Sumw2()

                    eventWeights = numpy.array(originalMCweights)
                    resol.GetEventWeights('Rotated',mcTree.GetV1(),eventWeights,mcTree.GetSelectedRows(),m,lowEnergy,upEnergy,int(startTime),0)

                    mcTemp = mcSmeared.Clone('run_%s_%s_mc'%(dataRunNumber,plotId))
                    mcTemp.Scale(dataTemp.Integral()/mcTemp.Integral())
                    sdMCTemp = ROOT.TH1D('run_%s_%s_mc_sd'%(dataRunNumber,plotId),'',nBinsSD,lowSD,upSD)
                    sdMCTemp.FillN(mcTree.GetSelectedRows(),mcTree.GetV3(),eventWeights)#mcTree.GetV2())
                    sdMCTemp.Scale(sdDataTemp.Integral()/sdMCTemp.Integral())

                    diffTemp = dataTemp.Clone(dataTemp.GetName()+'_diff')
                    diffTemp.Divide(mcTemp)
                    for b in range(nBins+1):
                        ratioXeneHisto.Fill(diffTemp.GetBinCenter(b),diffTemp.GetBinContent(b))
                    
                    dataHisto.Add(dataTemp)#,float(weekLivetime))
                    mcHisto.Add(mcTemp)#,float(weekLivetime))

                    sdDataHisto.Add(sdDataTemp)
                    sdMCHisto.Add(sdMCTemp)

                    outFile.cd()
                    dataTemp.Write()
                    mcTemp.Write()
                    sdDataTemp.Write()
                    sdMCTemp.Write()

                    week = (int(weekIndex)+1)//nWeeksCombo
                    weeklyPolFit[week]['MC'].Add(mcTemp)
                    weeklyPolFit[week]['data'].Add(dataTemp)

                    sdDataTemp.Sumw2()
                    sdMCTemp.Sumw2()
                    sdDataTemp.Add(sdMCTemp,-1)
                    sdDataTemp.Divide(sdMCTemp)
                    totAbsRes = 0.
                    totResErr = 0.
                    for b in range(1,sdDataTemp.GetNbinsX()+1):
                        if not sdDataTemp.GetBinError(b) > 0:
                            continue
                        content = sdDataTemp.GetBinContent(b)
                        invError = 1./sdDataTemp.GetBinError(b)**2
                        totAbsRes += ROOT.TMath.Abs(content)*invError
                        totResErr += invError
                    n = resSDGraph.GetN()
                    resSDGraph.SetPoint(n,float(dataRunNumber),totAbsRes/totResErr)
                    resSDGraph.SetPointError(n,0,(1/totResErr)**0.5)

                    dataTemp.Divide(mcTemp)
                    if fitOn:
                        #for i in range(5):
                        dataTemp.Fit('pol1','QR0','',lowEnergyFit,upEnergyFit)

                        fitFunction = dataTemp.GetFunction('pol1')
                        p0 = fitFunction.GetParameter(0)
                        p1 = fitFunction.GetParameter(1)*1e6
                        p0_error = fitFunction.GetParError(0)
                        p1_error = fitFunction.GetParError(1)*1e6
                        polfitHisto0.Fill(p0)
                        polfitHisto1.Fill(p1)

                        nPoint = polfitGraph0.GetN()
                        polfitGraph0.SetPoint(nPoint,float(dataRunNumber),p0)
                        polfitGraph0.SetPointError(nPoint,0,p0_error)
                        nPoint = polfitGraph1.GetN()
                        polfitGraph1.SetPoint(nPoint,float(dataRunNumber),p1)
                        polfitGraph1.SetPointError(nPoint,0,p1_error)

                    del mcSmeared
                    if dataTreeFile:
                        dataTreeFile.Close()
                    dataFile.Close()                    

                #del mcSmeared
                if mcTreeFile:
                    mcTreeFile.Close()
                mcFile.Close()

            for week in weeklyPolFit:
                dataTemp = weeklyPolFit[week]['data']
                mcTemp = weeklyPolFit[week]['MC']
                dataTemp.Divide(mcTemp)
                if fitOn:
                    #for i in range(5):
                    dataTemp.Fit('pol1','QR0','',lowEnergyFit,upEnergyFit)
                    fitFunction = dataTemp.GetFunction('pol1')
                    p0 = fitFunction.GetParameter(0)
                    p1 = fitFunction.GetParameter(1)*1e6
                    p0_error = fitFunction.GetParError(0)
                    p1_error = fitFunction.GetParError(1)*1e6

                    polfitHisto0week.Fill(p0)
                    polfitHisto1week.Fill(p1)

                    nPoint = polfitGraph0week.GetN()
                    polfitGraph0week.SetPoint(nPoint,float(week),p0)
                    polfitGraph0week.SetPointError(nPoint,0,p0_error)
                    nPoint = polfitGraph1week.GetN()
                    polfitGraph1week.SetPoint(nPoint,float(week),p1)
                    polfitGraph1week.SetPointError(nPoint,0,p1_error)
                    
            outFile.cd()

            dataHisto.Write()
            mcHisto.Write()
            sdDataHisto.Write()
            sdMCHisto.Write()

            resHisto.Add(dataHisto,mcHisto,1,-1)
            resHisto.Divide(mcHisto)
            resHisto.Write()
            
            resSDHisto.Add(sdDataHisto,sdMCHisto,1,-1)
            resSDHisto.Divide(sdMCHisto)
            resSDHisto.Write()

            for b in range(1,resHisto.GetNbinsX()+1):
                dataContent = dataHisto.GetBinContent(b)
                mcContent = mcHisto.GetBinContent(b)
                content = resHisto.GetBinContent(b)
                if dataContent >= 1 and mcContent >= 1:
                    invError = 1./resHisto.GetBinError(b)**2
                    totAbsRes += ROOT.TMath.Abs(resHisto.GetBinContent(b))*invError
                    totResErr += invError
            ratioHisto.Divide(dataHisto,mcHisto)
            ratioHisto.Fit('pol1','QR','',lowEnergyFit,upEnergyFit)
            ratioHisto.Write()

            ratioXeneHisto.Write()
            ratioXeneHisto.ProfileX().Write()

            resSDGraph.Write()

            polfitHisto0.Write()
            polfitHisto1.Write()
            polfitGraph0.Write()
            polfitGraph1.Write()

            polfitHisto0week.Write()
            polfitHisto1week.Write()
            polfitGraph0week.Write()
            polfitGraph1week.Write()


print 'Weighted mean of absolute residuals', totAbsRes/totResErr, (1/totResErr)**0.5
         
outFile.Close()
