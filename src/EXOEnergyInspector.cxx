#include "EXOEnergyInspector.hh"

ClassImp(EXOEnergyInspector)

EXOEnergyInspector::EXOEnergyInspector(const EXOSourceRunsPolishedInfo* sourceRunsInfo, const char* flavorName)
{
  SetVerboseLevel(0);
  SetSourceRunsInfo(sourceRunsInfo);
  SetFlavorName(flavorName);
  rootFile = 0;
}

EXOEnergyInspector::~EXOEnergyInspector()
{
  //if(rootFile)
  //  rootFile->Close();
}

void EXOEnergyInspector::SetVerboseLevel(int verbose)
{
  fVerboseLevel = verbose;
}

void EXOEnergyInspector::SetSourceRunsInfo(const EXOSourceRunsPolishedInfo* sourceRunsInfo)
{
  fSourceRunsInfo = sourceRunsInfo;
}

void EXOEnergyInspector::SetFlavorName(const char* flavorName)
{
  fFlavorName = flavorName;
}

bool EXOEnergyInspector::SetOutputFile(const char* filename, const char* mode)
{
  std::string newName = filename;
  if(rootFile)
  {
    if(!rootFile->IsZombie())
    {
      std::string curName = rootFile->GetName();
      if(newName == curName)
        return false;
    }
    else
    {
      rootFile->Close();
      rootFile = 0;
    }
  }

  rootFile = new TFile(filename,mode);
  return true;
}

std::vector<TGraph> EXOEnergyInspector::PlotWeeklyParameters(const std::string& channel, const std::string& parType)
{
  std::vector<TGraph> plots;
  for(int multInt = 1; multInt <= 2; multInt++)
  {
    std::vector<TGraph> multPlots = PlotWeeklyParametersForMultiplicity(multInt,channel,parType);
    plots.insert(plots.end(),multPlots.begin(),multPlots.end());
  }
  return plots;
}
  
std::vector<TGraph> EXOEnergyInspector::PlotWeeklyParametersForMultiplicity(int multInt, const std::string& channel, const std::string& parType)
{
  std::vector<TGraph> plots(3);
  
  const std::vector<std::string> weeks = fSourceRunsInfo->GetListOf("WeekIndex");
  const std::vector<std::string> startTime = fSourceRunsInfo->GetListOf("StartTimeSec");

  std::map<int, long> weekTimes;
  for(size_t i = 0; i < weeks.size(); i++)
  {
    int weekInt = atoi(weeks.at(i).c_str());
    if(weekTimes.count(weekInt) > 0)
      continue;
    weekTimes.insert(std::pair<int, long>(weekInt,atol(startTime.at(i).c_str())));
  }
  
  for(std::map<int, long>::iterator weekTime = weekTimes.begin(); weekTime != weekTimes.end(); weekTime++)
  {
    EXOEnergyMCBasedFit* dbTable = GetCalibrationFor(EXOEnergyMCBasedFit, EXOEnergyMCBasedFitHandler,fFlavorName,EXOTimestamp(weekTime->second,0));
    for(int p = 0; p < 3; p++)
    {
      double parValue = dbTable->GetParByName(Form("%s_%s_p%d",channel.c_str(),parType.c_str(),p),multInt);
      int n = plots.at(p).GetN();
      plots[p].SetPoint(n,weekTime->first,parValue);
    }
  }

  if(rootFile && !rootFile->IsZombie())
  {
    TCanvas canvas;
    for(int p = 0; p < 3; p++)
    {
      rootFile->cd();
      plots[p].SetMarkerStyle(20);
      plots[p].SetMarkerColor(p+1);
      plots.at(p).Write(Form("%s_%s_p%d_%s",channel.c_str(),parType.c_str(),p,(multInt == 1) ? "ss" : "ms"));
      canvas.cd();
      plots.at(p).Draw(Form("%sp",(p==0)? "A" : ""));
    }
    rootFile->cd();
    canvas.Write(Form("%s_%s_ps_%s",channel.c_str(),parType.c_str(),(multInt == 1) ? "ss" : "ms"));
  }

  return plots; 
}
