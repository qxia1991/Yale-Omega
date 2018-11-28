
import ROOT, settings
import sys
import PolishSourceRunInfoFromDatabases as psrifd

settings.init()

ROOT.gSystem.Load('libEXOCalibUtilities')
ROOT.gSystem.Load('../lib/libEXOEnergy.so')

channel = 'Rotated'#'Ionization'#'Ionization'

def GetSourceRunsInfo():

    infoRuns = ROOT.EXOSourceRunsPolishedInfo(settings.SourceRunsFileName)

    infoRuns.CutExact('SourcePositionS','0',False)
    infoRuns.CutExact('SourcePositionS','17',False)
    infoRuns.CutExact('ManualFlag','NONE')
    infoRuns.CutExact('Quality','BAD',False)
    infoRuns.AddExactCondition('SourceName','Cs-137',True)
    infoRuns.AddExactCondition('SourceName','Ra-226',True)
    infoRuns.AddExactCondition('SourceName','Co-60',True)
    infoRuns.ClearExceptRuns()
    infoRuns.CutDoubleComparison('WeekIndex',settings.minWeek,True)
    infoRuns.CutDoubleComparison('WeekIndex',settings.maxWeek,False)

    infoRuns.SetMetadata('WeeklyFitFileName','%sfit_week_[WeekIndex].root'%(settings.CalibrationOutput))
    infoRuns.SetMetadata('AverageCampaignFitFileName','%sweighted_calib_cpg.root'%(settings.CalibrationOutput)) #weighted_calib_cpg_fit.root')

    return infoRuns

def NewDBInfo():
    
    result = {}
    result['Multiplicity'] = ''
    result['Angle'] = 0.
    for p in ['0','1','2']:
        result['Correlation_p%s'%(p)] = 0.
        result['Correlation_p%s_Error'%(p)] = 0.
        if p in ['0']:
            result['TriggerEfficiency_Correlation_p%s'%(p)] = 0.
            result['TriggerEfficiency_Correlation_p%s_Error'%(p)] = 0.            
        for ris in ['Rotated','Ionization','Scintillation']:
            for crt in ['Calibration','Resolution','TriggerEfficiency']:
                if crt == 'TriggerEfficiency' and p == '2':
                    continue
                result['%s_%s_p%s'%(ris,crt,p)] = 0.
                result['%s_%s_p%s_Error'%(ris,crt,p)] = 0.

    return result

def AddWeekInfo(week,info):

    psrifd.MakeWeekIndexStartTime()

    if week == 'all':
        info['WeekIndex'] = -1
        startTime = ROOT.TTimeStamp(psrifd.weekIndexStartTime[0])
        endTime = ROOT.TTimeStamp(psrifd.weekIndexStartTime[-1])
        info['start_time'] = startTime.AsString('s')
        info['start_time_sec'] = startTime.GetSec()
        info['end_time'] = endTime.AsString('s')
        info['end_time_sec'] = endTime.GetSec()        
        return 

    week = int(week)
    info['WeekIndex'] = week
    startTime = ROOT.TTimeStamp(psrifd.weekIndexStartTime[week])
    info['start_time'] = startTime.AsString('s')
    info['start_time_sec'] = startTime.GetSec()
    endTime = ROOT.TTimeStamp(psrifd.weekIndexStartTime[week+1])
    info['end_time'] = endTime.AsString('s')
    info['end_time_sec'] = endTime.GetSec()

def GetWeekAngle(week,mult):

    psrifd.MakeWeekIndexStartTime()
    timeSec = int((psrifd.weekIndexStartTime[week] + psrifd.weekIndexStartTime[week+1])/2.)
    peak = ROOT.EXOCalibManager.GetCalibManager().getCalib(ROOT.EXOEnergyMCBasedFitHandler.GetHandlerName(),settings.calibFlavor1,timeSec)
    
    return peak.GetParByName("Angle",(2,1)[mult == 'ss'])
    
def AssociateFitInfo():

    corresp = {}
    for p in range(3):
        corresp['calib_p%i'%p] = '%s_Calibration_p%i'%(channel,p)
        corresp['resol_p%i'%p] = '%s_Resolution_p%i'%(channel,p)
        if p < 2:
            corresp['trigeff_p%i'%p] = '%s_TriggerEfficiency_p%i'%(channel,p)
        
    return corresp

def FillBlankWeeks(fitResults):
    fittedWeeks = []
    for week in fitResults:
        fittedWeeks.append(week)
    fittedWeeks.sort()
    maxWeek = max(fittedWeeks)
    print fittedWeeks

    psrifd.MakeWeekIndexStartTime()
    for week in range(len(psrifd.weekIndexStartTime)):
        if week > maxWeek:
            break
        if week == maxWeek:
            endTime = ROOT.TTimeStamp(psrifd.weekIndexStartTime[-1])
            for mult in ['ss','ms']:
                fitResults[week][mult]['end_time'] = endTime.AsString('s')
                fitResults[week][mult]['end_time_sec'] = endTime.GetSec()
        if not week in fittedWeeks:
            nearestWeek = min(range(len(fittedWeeks)), key = lambda i: abs(fittedWeeks[i]-week))
            fitResults[week] = {}
            print week, fittedWeeks[nearestWeek]
            for mult in ['ss','ms']:
                fitResults[week][mult] = {}
                for var in fitResults[fittedWeeks[nearestWeek]][mult]:
                    fitResults[week][mult][var] = fitResults[fittedWeeks[nearestWeek]][mult][var]
                AddWeekInfo(week,fitResults[week][mult])

def GetFitInfo(minuit, info, corresp):

    name = ROOT.TString()
    value = ROOT.Double()
    error = ROOT.Double()
    low = ROOT.Double()
    up = ROOT.Double()
    iu = ROOT.Long()
    n = minuit.GetNumPars()
    for i in range(n):
        minuit.mnpout(i,name,value,error,low,up,iu)
        #print 'Par', i, name,value,error,low,up,iu
        varName = str(name.Data())
        varValue = float(value)
        if varName.find('resol') >= 0 and varValue < 0:
            varValue = -varValue
        varError = float(error)
        dbName = corresp[varName]
        dbNameError = dbName + '_Error'
        info[dbName] = varValue
        info[dbNameError] = varError
 
    return

def GetAverageCampaignFitResults(filenames):

    fitResults = {}
    processedFilenames = []

    for i in range(filenames.size()):
        filename = str(filenames.at(i))
        if filename in processedFilenames:
            continue

        for mult in ['ss','ms']:
            fitResults[mult] = NewDBInfo()
            fitResults[mult]['Multiplicity'] = mult.upper()

            rootFile = ROOT.TFile(filename.replace('.root','_%s.root'%(mult)),'read')
            if rootFile.IsZombie():
                fitResults.pop(mult)
                continue
            
            minuit = rootFile.Get('MINUIT')
            GetFitInfo(minuit,fitResults[mult],AssociateFitInfo())
            AddWeekInfo('all',fitResults[mult])

            print 'avg', mult
            print fitResults[mult]

            rootFile.Close()

        processedFilenames.append(filename)

    return fitResults
       

def GetWeeklyFitResults(weeks,filenames):

    fitResults = {}
    processedWeeks = []
    for i in range(weeks.size()):
        week = int(weeks.at(i))
        if week in processedWeeks:
            continue

        filename = filenames.at(i)
        fitResults[week] = {}
        for mult in ['ss','ms']:
            fitResults[week][mult] = NewDBInfo()
            fitResults[week][mult]['Multiplicity'] = mult.upper()

            rootFile = ROOT.TFile(filename.replace('.root','_%s.root'%(mult)),'read')
            if rootFile.IsZombie():
                fitResults.pop(week)
                break

            minuit = rootFile.Get('MINUIT')
            
            GetFitInfo(minuit,fitResults[week][mult],AssociateFitInfo())
            AddWeekInfo(week,fitResults[week][mult])
            fitResults[week][mult]['Angle'] = GetWeekAngle(week,mult)

            print week, mult
            #print fitResults[week][mult]

            rootFile.Close()

        processedWeeks.append(week)

    return fitResults

def GetWeeklyFitResiduals(weeks,filenames):

    fitResiduals = {}
    processedWeeks = []
    for i in range(weeks.size()):
        week = int(weeks.at(i))
        if week in processedWeeks:
            continue

        filename = filenames.at(i)
        fitResiduals[week] = {}
        for mult in ['ss','ms']:
            fitResiduals[week][mult] = {}

            rootFile = ROOT.TFile(filename.replace('.root','_%s.root'%(mult)),'read')
            if rootFile.IsZombie():
                fitResiduals.pop(week)
                break
            
            hData = rootFile.Get('FitHistoEntry_histo_all_fit_week_%03d_%s_data'%(week,mult))
            hMC = rootFile.Get('FitHistoEntry_histo_all_fit_week_%03d_%s_smearedmc'%(week,mult))

            hData.Add(hMC,-1.)
            hData.Divide(hMC)

            absSum = 0.
            errorTot = 0.
            for b in range(1,hData.GetNbinsX()+1):
                binError = hData.GetBinError(b)
                if binError <= 0:
                    continue
                if hData.GetBinContent(b) <= -1:
                    continue
                weight = 1./binError**2
                absSum += ROOT.TMath.Abs(hData.GetBinContent(b))*weight
                errorTot += weight

            fitResiduals[week][mult]['value'] = absSum/errorTot
            fitResiduals[week][mult]['error'] = (1./errorTot)**0.5

            rootFile.Close()

        processedWeeks.append(week)

    return fitResiduals

def WriteAverageCampaignFitResultsToDBFile(results,filename):

    outFile = open(filename,'w')
    
    for mult in sorted(results):
        line = ''
        for var in sorted(results[mult]):
            #if var.find('_Error') >= 0:
            #    continue
            line += '%s;'%(var)
        line = line[:len(line)-1]
    outFile.write(line+'\n')

    for mult in sorted(results):
        line = ''
        for var in sorted(results[mult]):
            #if var.find('_Error') >= 0:
            #    continue
            line += '%s;'%(str(results[mult][var]))
        line = line[:len(line)-1]
        outFile.write(line+'\n')

    outFile.close()


def WriteWeeklyFitResultsToDBFile(results,filename):

    outFile = open(filename,'w')
    
    for week in sorted(results):
        for mult in sorted(results[week]):
            line = ''
            for var in sorted(results[week][mult]):
                if var.find('_Error') >= 0:
                    continue
                line += '%s;'%(var)
            line = line[:len(line)-1]
    outFile.write(line+'\n')

    for week in sorted(results):
        for mult in sorted(results[week]):
            line = ''
            for var in sorted(results[week][mult]):
                if var.find('_Error') >= 0:
                    continue
                line += '%s;'%(str(results[week][mult][var]))
            line = line[:len(line)-1]
            outFile.write(line+'\n')

    outFile.close()
    
def WriteWeeklyFitResultsInDBFormat(infoRuns,filename):

    weeks = infoRuns.GetListOf('WeekIndex')
    filenames = infoRuns.GetListOf('WeeklyFitFileName')
    
    fitResults = GetWeeklyFitResults(weeks,filenames)
    FillBlankWeeks(fitResults)
    WriteWeeklyFitResultsToDBFile(fitResults,filename)

def WriteAverageFitResultsInDBFormat(infoRuns,filename):

    filenames = infoRuns.GetListOf('AverageCampaignFitFileName')
    
    fitResults = GetAverageCampaignFitResults(filenames)
    WriteAverageCampaignFitResultsToDBFile(fitResults,filename)

def MakeStabilityPlots(infoRuns,filename):

    weeks = infoRuns.GetListOf('WeekIndex')
    filenames = infoRuns.GetListOf('WeeklyFitFileName')
    avgnames = infoRuns.GetListOf('AverageCampaignFitFileName')
    
    fitResults = GetWeeklyFitResults(weeks,filenames)
    fitResiduals = GetWeeklyFitResiduals(weeks,filenames)
    FillBlankWeeks(fitResults)

    avgResults = GetAverageCampaignFitResults(avgnames)    

    MakeStabilityPlotsWith(fitResults,avgResults,fitResiduals,filename)

def MakeStabilityPlotsWith(results,avgs,residuals,filename):
    
    outFile = ROOT.TFile(filename,'recreate')

    graphs = {'Calibration':{},'Resolution':{}}
    for kind in graphs:
        graphs[kind] = {}
        for p in range(2):
            graphs[kind][p] = PlotGraphVsWeekOf('%s_%s_p%i'%(channel,kind,p),results)
            for mult in graphs[kind][p]:
                outFile.cd()
                graphs[kind][p][mult].Write('%s_%s_p%i_%s'%(channel,kind,p,mult))

    graphs = {'ss':ROOT.TGraphErrors(),'ms':ROOT.TGraphErrors()}
    histos = {'ss':ROOT.TH1D('','',5000,0,0.5),'ms':ROOT.TH1D('','',5000,0,0.5)}
    for mult in graphs:
        for week in residuals:
            n = graphs[mult].GetN()
            x = float(week)
            y = residuals[week][mult]['value']
            ey = residuals[week][mult]['error']
            graphs[mult].SetPoint(n,x,y)
            graphs[mult].SetPointError(n,0,ey)
            histos[mult].Fill(y)
        outFile.cd()
        graphs[mult].Write('Weekly_%s_Residuals_%s'%(channel,mult))
        histos[mult].Write('Histo_Weekly_%s_Residuals_%s'%(channel,mult))

    graphs = PlotCompareWeekAndAvgResolution(results,avgs)
    for mult in graphs:
        for ene in graphs[mult]:
            outFile.cd()
            graphs[mult][ene].Write('AvgWeekResolComp_%d_%s'%(ene,mult))

    outFile.Close()

def PlotCompareWeekAndAvgResolution(results,avgs):

    graphs = {'ss':{},'ms':{}}
    for mult in graphs:
        for ene in [500,1000,2000,2458,3500]:
            graphs[mult][ene] = ROOT.TGraph()

    aps = {}
    #aps['ss'] = []#[0.7139,23.1181,4.45e-8]
    #aps['ms'] = []#[0.316589,26.8157,0.0101627]
    for mult in ['ss','ms']:
        aps[mult] = []
        for p in range(3):
            aps[mult].append(avgs[mult]['%s_Resolution_p%d'%(channel,p)])

    for week in sorted(results):
        for mult in sorted(results[week]):

            ps = []
            for p in range(2):
                ps.append(results[week][mult]['%s_Resolution_p%d'%(channel,p)])
            ap = aps[mult]
            for ene in [500,1000,2000,2458,3500]:
                n = graphs[mult][ene].GetN()
                x = float(week)
                wr = (ps[0]*ps[0]*ene + ps[1]*ps[1])**0.5 / ene
                ar = (ap[0]*ap[0]*ene + ap[1]*ap[1] + ap[2]*ap[2]*ene*ene)**0.5 / ene
                #print ene, wr, ar
                y = ROOT.TMath.Abs((wr-ar)/ar)
                graphs[mult][ene].SetPoint(n,x,y)
            
    return graphs

def PlotGraphVsWeekOf(varName,results):
    
    graphs = {'ss':ROOT.TGraphErrors(),'ms':ROOT.TGraphErrors()}

    for week in sorted(results):
        for mult in sorted(results[week]):
            n = graphs[mult].GetN()
            x = float(week)
            y = results[week][mult][varName]
            ey = results[week][mult][varName+'_Error']
            graphs[mult].SetPoint(n,x,y)
            graphs[mult].SetPointError(n,0,ey)   

    return graphs
    

if __name__ == '__main__':

    infoRuns = GetSourceRunsInfo()
    WriteWeeklyFitResultsInDBFormat(infoRuns,'weeklyFitResultsDB-Phase2-061217.txt')
    #WriteAverageFitResultsInDBFormat(infoRuns,'averageFitResultsDB-Restart-053116.txt')
#MakeStabilityPlots(infoRuns,'stabilityPlots-2014-Exotica.root')
