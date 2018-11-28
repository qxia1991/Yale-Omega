
#include "EXOEnergyCalibrator.hh"

ClassImp(EXOEnergyCalibrator)

int EXOEnergyCalibrator::fNbins[3] = {0,760,760};//{0,860,1000};//fix week 17 = {0,580,1000}; std real noise = {0,600,1000}; white noise (ss = ms) = {0,600,800};
double EXOEnergyCalibrator::fLowEnergy[3] = {0,1200,1200};//{0,700,0};//fix week 17 = {0,1100,0}; std real noise = {0,1000,0}; white noise (ss = ms) = {0,1000,1000};
double EXOEnergyCalibrator::fUpEnergy[3] = {0,5000,5000};//{0,5000,5000};

//int EXOEnergyCalibrator::fNbins[3] = {0,480,740};//{0,860,1000};//fix week 17 = {0,580,1000}; std real noise = {0,600,1000}; white noise (ss = ms) = {0,600,800};
//double EXOEnergyCalibrator::fLowEnergy[3] = {0,600,300};//{0,700,0};//fix week 17 = {0,1100,0}; std real noise = {0,1000,0}; white noise (ss = ms) = {0,1000,1000};
//double EXOEnergyCalibrator::fUpEnergy[3] = {0,3000,4000};//{0,5000,5000};

EXOEnergyCalibrator::EXOEnergyCalibrator(const EXOSourceRunsPolishedInfo* sourceRunsInfo)
{
  SetVerboseLevel(0);
  SetSourceRunsInfo(sourceRunsInfo);
  SetCalibrationChannel("Rotated");
  SetInitialCalibPars(10,0.75);
  SetInitialResPars(0.1,20.);
  SetUseAngleFromFile(false);
  SetCalibFlavor("2014-v3-weekly");
  SetIsCalibrated(false);
  fIgnoreLimits = NULL;
  fFitOnlyPeaks = false;
}

EXOEnergyCalibrator::~EXOEnergyCalibrator()
{
}

void EXOEnergyCalibrator::SetVerboseLevel(int verbose)
{
  fVerboseLevel = verbose;
}

void EXOEnergyCalibrator::SetCalibrationChannel(std::string channel)
{
  fCalibrationChannel = channel;
}

void EXOEnergyCalibrator::SetSourceRunsInfo(const EXOSourceRunsPolishedInfo* sourceRunsInfo)
{
  fSourceRunsInfo = sourceRunsInfo;
}

bool EXOEnergyCalibrator::FitWeeklyTh(const char* weeklyOutputFileName, const char* weekWildCard)
{
  if(fVerboseLevel > 0)
    std::cout << "Starting weekly Th fits..." << std::endl;

  EXOSourceRunsPolishedInfo thRunsInfo = (*fSourceRunsInfo);
  thRunsInfo.CutExact("SourceName","Th-228");

  fWeeklyOutputName = weeklyOutputFileName;
  fWeekWildCard = weekWildCard;

  fFitCalibrationType = "weekly";
  fFitFunctionType = "linear";
    
  const std::vector<std::string> thWeeks = thRunsInfo.GetListOf("WeekIndex");

  std::set<std::string> weeks(thWeeks.begin(),thWeeks.end());

  for(std::set<std::string>::iterator week = weeks.begin(); week != weeks.end(); week++)
  {
    FitForWeek(*week,thRunsInfo);
  }

  return true;
}

bool EXOEnergyCalibrator::FitWeeklySources(const char* weeklyOutputFileName, const char* weekWildCard)
{
  if(fVerboseLevel > 0)
    std::cout << "Starting weekly sources fits..." << std::endl;

  fWeeklyOutputName = weeklyOutputFileName;
  fWeekWildCard = weekWildCard;

  fFitCalibrationType = "weekly";
  //fFitFunctionType = "linear";
    
  const std::set<std::string> weeks = fSourceRunsInfo->GetSetOf("WeekIndex");

  for(std::set<std::string>::iterator week = weeks.begin(); week != weeks.end(); week++)
  {
   FitForWeek(*week,*fSourceRunsInfo);
  }

  return true;
}

bool EXOEnergyCalibrator::FitCalibrationCampaigns(const char* outputFileName, const char* weeklyOutputFileName, const char* weekWildCard)
{
  if(fVerboseLevel > 0)
    std::cout << "Starting calibration campaigns fit..." << std::endl;

  EXOSourceRunsPolishedInfo calibCpgRunsInfo = (*fSourceRunsInfo);
  calibCpgRunsInfo.CutExact("CampaignIndex","00",false);
  calibCpgRunsInfo.CutExact("CampaignLivetime","0.0",false);
  

  fWeeklyOutputName = weeklyOutputFileName;
  fWeekWildCard = weekWildCard;
  fOutputName = outputFileName;
  
  fFitCalibrationType = "campaigns";
  fFitFunctionType = "quadratic";
  
  fNbins[0] = 0;
  fNbins[1] = 410;
  fNbins[2] = 800;
  fLowEnergy[0] = 0;
  fLowEnergy[1] = 950;
  fLowEnergy[2] = 0;
  fUpEnergy[0] = 0;
  fUpEnergy[1] = 3000;
  fUpEnergy[2] = 4000;
  
  const std::set<std::string> weeks = calibCpgRunsInfo.GetSetOf("WeekIndex");
  //std::set<std::string> weeks(allWeeks.begin(),allWeeks.end());

  /*
  const std::vector<std::string> weeks = calibCpgRunsInfo.GetListOf("WeekIndex");
  const std::vector<std::string> livetimes = calibCpgRunsInfo.GetListOf("WeekLivetime");
  std::set<std::string> weekSet;
  for(size_t i = 0; i < weeks.size(); i++)
  {
    if(atof(livetimes.at(i).c_str()) > 0.)
      weekSet.insert(weeks.at(i));
  }
  */
  
  if(!SetWeeklyCalibrationFunctions(weeks))
    return false;
  
  for(int multInt = 1; multInt <= 2; multInt++)
  {
    FitForMultiplicity(multInt,calibCpgRunsInfo);
  }

  return true;
}

bool EXOEnergyCalibrator::FitNoisePeriods(const char* outputFileName, const char* weeklyOutputFileName, const char* weekWildCard)
{
  if(fVerboseLevel > 0)
    std::cout << "Starting noise periods fit..." << std::endl;

  EXOSourceRunsPolishedInfo noiseRunsInfo = (*fSourceRunsInfo);
  //calibCpgRunsInfo.CutExact("NoisePeriodIndex","00",false);
  //calibCpgRunsInfo.CutExact("CampaignLivetime","0.0",false);
  noiseRunsInfo.SetNoiseLevelPeriods();
  
  fWeeklyOutputName = weeklyOutputFileName;
  fWeekWildCard = weekWildCard;
  fOutputName = outputFileName;
  
  fFitCalibrationType = "noise";
  fFitFunctionType = "quadratic";
    
  const std::set<std::string> weeks = noiseRunsInfo.GetSetOf("WeekIndex");
    
  if(!SetWeeklyCalibrationFunctions(weeks))
    return false;
  
  for(int multInt = 1; multInt <= 2; multInt++)
  {
    FitForMultiplicity(multInt,noiseRunsInfo);
  }

  return true;
}

bool EXOEnergyCalibrator::FitSpatialDistributions(const char* outputFileName, const char* weeklyOutputFileName, const char* weekWildCard)
{
  if(fVerboseLevel > 0)
    std::cout << "Starting spatial distributions fit..." << std::endl;

  EXOSourceRunsPolishedInfo sdRunsInfo = (*fSourceRunsInfo);
  //calibCpgRunsInfo.CutExact("NoisePeriodIndex","00",false);
  //calibCpgRunsInfo.CutExact("CampaignLivetime","0.0",false);
  sdRunsInfo.SetSpatialDistribution();
  
  fWeeklyOutputName = weeklyOutputFileName;
  fWeekWildCard = weekWildCard;
  fOutputName = outputFileName;
  
  fFitCalibrationType = "spatial-distribution";
  fFitFunctionType = "quadratic";
    
  const std::set<std::string> weeks = sdRunsInfo.GetSetOf("WeekIndex");
    
  if(!SetWeeklyCalibrationFunctions(weeks))
    return false;
  
  for(int multInt = 1; multInt <= 2; multInt++)
  {
    FitForMultiplicity(multInt,sdRunsInfo);
  }

  return true;
}

bool EXOEnergyCalibrator::FitForWeek(const std::string& week, const EXOSourceRunsPolishedInfo& runsInfo)
{
  int weekInt = atoi(week.c_str());
  std::string weekStr = Form("%03d",weekInt);
  if(fVerboseLevel > 0)
    std::cout << "Fitting week " << weekStr << std::endl;
  
  EXOSourceRunsPolishedInfo weekRunsInfo = runsInfo;
  weekRunsInfo.CutExact("WeekIndex",week);

  const std::set<std::string> weekSources = weekRunsInfo.GetSetOf("SourceName");

  fFitFunctionType = "linear";
  //if(weekSources.count("Th-228") > 0 && (weekSources.count("Co-60")>0 || weekSources.count("Ra-226")>0))
  //  fFitFunctionType = "quadratic";
  //else if(weekSources.count("Co-60") > 0 &&  (weekSources.count("Th-228")>0 || weekSources.count("Ra-226")>0 || weekSources.count("Cs-137")>0))
  //  fFitFunctionType = "quadratic";

  fOutputName = fWeeklyOutputName;
  fOutputName = fOutputName.ReplaceAll(fWeekWildCard.Data(),weekStr.c_str());

  for(int multInt = 1; multInt <= 2; multInt++)
  {
    FitForMultiplicity(multInt,weekRunsInfo);
  }

  return true;
}

bool EXOEnergyCalibrator::FitForMultiplicity(const int multInt, const EXOSourceRunsPolishedInfo& runsInfo)
{
  std::string multStr = (multInt == 1) ? "ss" : "ms";
  if(fVerboseLevel > 0)
    std::cout << "Fitting events with multiplicity " << multStr << std::endl;

  TString fMultOutputName = fOutputName;
  if(fMultOutputName.Contains(".root"))
    fMultOutputName = fMultOutputName.ReplaceAll(".root",Form("_%s.root",multStr.c_str()));
  else
    fMultOutputName = Form("%s_%s.root",fMultOutputName.Data(),multStr.c_str());

  std::string fitHistoName = fMultOutputName.Data();
  size_t posSlash = fitHistoName.rfind("/");
  if(posSlash == std::string::npos)
    posSlash = -1;
  size_t posRoot = fitHistoName.rfind(".root");
  fitHistoName = fitHistoName.substr(posSlash+1,posRoot-posSlash-1);

  std::string division = "";
  std::string weight = "";

  if(fFitCalibrationType == "campaigns")
  {
    division = "CampaignIndex";
    weight = "CampaignLivetime";
  }
  else if(fFitCalibrationType == "noise")
  {
    division = "NoiseLevel";//"NoiseIndex";
    weight = "NoiseLevelLivetime";//"NoiseLivetime";
  }
  else if(fFitCalibrationType == "spatial-distribution")
  {
    division = "SpatialDistribution";//"NoiseIndex";
    weight = "SpatialDistributionLivetime";//"NoiseLivetime";
  }
  
  EXOEnergyMCBasedFit1D energyFitter;
  energyFitter.SetVerboseLevel(fVerboseLevel);
  
  if(!EXOEnergyUtils::AddMCToFitterForMultiplicity(energyFitter,runsInfo,multInt))
    return false;
  if(!AddData(energyFitter,runsInfo,multInt))
    return false;
  if(!SetWeightedHisto(Form("histo_all_%s",fitHistoName.c_str()),energyFitter,runsInfo,multInt,division,weight))
    return false;
  if(!SetFitFunctions(energyFitter))
    return false;

  if(fFitOnlyPeaks)
  {
    
    energyFitter.FitOnlyPeaks();
  }
  else if(fIgnoreLimits)
  {
    if(!energyFitter.IgnoreBins(fIgnoreLimits->first,fIgnoreLimits->second))
      return false;
  }
  
  energyFitter.BinMCEnergy(1);

  if(energyFitter.ShouldSetTriggerEffFunction() && energyFitter.GetFunction("trigeff"))
  {
    energyFitter.SetAllFitCalculations(false);
    if(!energyFitter.ExecuteFit(2,"SIMPLEX",1.))
      return false;
    int n = energyFitter.GetFunction("trigeff")->GetNpar();
    for(int i = 0; i < n; i++)
      energyFitter.GetFunction("trigeff")->FixParameter(i,energyFitter.GetFunction("trigeff")->GetParameter(i));
    //for(int i = 0; i < energyFitter.GetFunction("calib")->GetNpar(); i++)
    //{
    // energyFitter.GetFunction("calib")->SetParameter(i,energyFitter.GetFunction("calib")->GetParError(i));
    // energyFitter.GetFunction("calib")->SetParError(i,std::fabs(energyFitter.GetFunction("calib")->GetParError(i)));
    //}
    //for(int i = 0; i < energyFitter.GetFunction("resol")->GetNpar(); i++)
    //{
    //  energyFitter.GetFunction("resol")->SetParameter(i,energyFitter.GetFunction("resol")->GetParError(i));    
    //  energyFitter.GetFunction("resol")->SetParError(i,std::fabs(energyFitter.GetFunction("resol")->GetParError(i)));
    //}
  }
    
  energyFitter.SetAllFitCalculations(true);
  if(!energyFitter.ExecuteFit(1,"SIMPLEX",1.))
    return false;
  if(!energyFitter.ExecuteFit(1,"MIGRAD",1.))
    return false;

  if(energyFitter.ShouldSetTriggerEffFunction() && energyFitter.GetFunction("trigeff"))
  { // after find calib and resol, re-fit threshold
    int n = energyFitter.GetFunction("trigeff")->GetNpar();
    for(int i = 0; i < n; i++)
      energyFitter.GetFunction("trigeff")->ReleaseParameter(i);
    
    energyFitter.SetAllFitCalculations(false);
    if(!energyFitter.ExecuteFit(2,"SIMPLEX",1.))
      return false;
    for(int i = 0; i < n; i++)
      energyFitter.GetFunction("trigeff")->FixParameter(i,energyFitter.GetFunction("trigeff")->GetParameter(i));
    
    energyFitter.SetAllFitCalculations(true);
    if(!energyFitter.ExecuteFit(1,"SIMPLEX",1.))
      return false;
    if(!energyFitter.ExecuteFit(1,"MIGRAD",1.))
      return false;
  }

  energyFitter.ApplyFittedParameters();
  energyFitter.SaveHistosIn(fMultOutputName.Data(),"RECREATE",4);

  energyFitter.Delete();

  return true;
}

bool EXOEnergyCalibrator::SetWeeklyCalibrationFunctions(const std::set<std::string>& weeks)
{
  fWeeklyCalibFunc.clear();
  for(std::set<std::string>::const_iterator week = weeks.begin(); week != weeks.end(); week++)
  {
    for(int multInt = 1; multInt <= 2; multInt++)
    {
      std::string multStr = (multInt == 1) ? "ss" : "ms";
      TF1 weekCalibFunc;
      if(!GetCalibrationFunctionForWeek(weekCalibFunc,*week,multStr))
        return false;
      fWeeklyCalibFunc.insert(std::pair<std::string, TF1>(Form("%s_%s",week->c_str(),multStr.c_str()),weekCalibFunc));
      std::cout << "Setting weekly calibration function: " << Form("%s_%s",week->c_str(),multStr.c_str()) << std::endl;
      //weekCalibFunc.Print();
    }
  }
  
  return true;
}

bool EXOEnergyCalibrator::GetCalibrationFunctionForWeek(TF1& calibFunc, const std::string& week, const std::string& multStr)
{
  int weekInt = atoi(week.c_str());
  std::string weekStr = Form("%03d",weekInt);
  
  TString weeklyFitFileName = fWeeklyOutputName;
  weeklyFitFileName = weeklyFitFileName.ReplaceAll(fWeekWildCard.Data(),weekStr.c_str());
  weeklyFitFileName = weeklyFitFileName.ReplaceAll(".root",Form("_%s.root",multStr.c_str()));

  TFile weeklyFitFile(weeklyFitFileName,"READ");
  if(weeklyFitFile.IsZombie())
  {
    std::cerr << "Could not load calibration functions from file " << weeklyFitFileName.Data() << std::endl;
    return false;
  }
  TF1 *weeklyCalibFunc = (TF1*) weeklyFitFile.Get("calib");
  if(!weeklyCalibFunc)
  {
    std::cerr << "Could not get calibration function calib from file " << weeklyFitFileName.Data() << std::endl;
    return false;
  }
  
  calibFunc = *weeklyCalibFunc;
  weeklyFitFile.Close();
  
  return true;  
}

bool EXOEnergyCalibrator::SetFitFunctions(EXOEnergyMCBasedFit1D& energyFitter)
{
  bool isSet = false;
  if(fFitFunctionType == "linear")
  {
    TF1 *calibFunc = new TF1("calib","[0]+[1]*x",0,10000);
    //double calibParsRot[] = {10,0.75};
    double calibParsCharge[] = {10.,1.};
    double calibParsScint[] = {-150.,0.4};
    double *calibPars = fCalibFitPars;
    if(fCalibrationChannel == "Ionization")
      calibPars = calibParsCharge;
    else if(fCalibrationChannel == "Scintillation")
      calibPars = calibParsScint;

    if (fIsCalibrated) {
      double pars[] = {0.0,1.0};
      calibPars = pars;
    }

    calibFunc->SetParameters(calibPars);
    calibPars[0] = std::fabs(calibPars[0]); calibPars[1] = std::fabs(calibPars[1]);
    //calibFunc->SetParLimits(0,-500,500);
    //calibFunc->SetParLimits(1,1e-2,1.2);
    calibFunc->SetParErrors(calibPars);
    energyFitter.SetFunction("calib",calibFunc);
    
    TF1 *resolFunc = new TF1("resol","sqrt([0]*[0]*x + [1]*[1])",0,10000);
    //double resolParsRot[] = {0.1,20.};//double resolPars[] = {0.1,20.};
    double resolParsCharge[] = {2.,10.};
    double resolParsScint[] = {2.7,50.};
    double *resolPars = fResFitPars;
    if(fCalibrationChannel == "Ionization")
      resolPars = resolParsCharge;
    else if(fCalibrationChannel == "Scintillation")
      resolPars = resolParsScint;

    resolFunc->SetParameters(resolPars);
    //resolFunc->SetParLimits(0,1.0,3.0);
    //resolFunc->SetParLimits(1,-500,500);
    resolFunc->SetParErrors(resolPars);
    energyFitter.SetFunction("resol",resolFunc);
    isSet = true;
  }

  if(fFitFunctionType == "quadratic")
  {
    TF1 *calibFunc = new TF1("calib","[0]+[1]*x+[2]*x*x",0,10000);
    double calibParsRot[] = {1.,1.,1e-6};
    if(fFitCalibrationType == "weekly")
    {
      calibParsRot[0] = 10.;
      calibParsRot[1] = 0.75;
      calibParsRot[2] = 1e-6;
    }
    double *calibPars = calibParsRot;
    calibFunc->SetParameters(calibPars);
    calibFunc->SetParErrors(calibPars);
    energyFitter.SetFunction("calib",calibFunc);
    
    TF1 *resolFunc = new TF1("resol","sqrt([0]*[0]*x + [1]*[1] + [2]*[2]*x*x)",0,10000);
    double resolParsRot[] = {0.7,40.,1e-7};
    double resolParsCharge[] = {2.,10.,0.001};
    double *resolPars = resolParsRot;
    if(fCalibrationChannel == "Ionization")
      resolPars = resolParsCharge;
    resolFunc->SetParameters(resolPars);
    resolFunc->SetParErrors(resolPars);
    //resolFunc->SetParLimits(0,1.0,3.0);
    //resolFunc->SetParLimits(1,0.1,100);
    //resolFunc->SetParLimits(2,1e-9,1e-3);
    energyFitter.SetFunction("resol",resolFunc);
    isSet = true;
  }

  if(energyFitter.ShouldSetTriggerEffFunction())
  {
    TF1 *trigEffFunc = new TF1("trigeff","0.5*(1+TMath::Erf(0.7071067811865474*(x-[0])/[1]))",0,10000);
    double trigEffParsRot[] = {1250.,150.}; //{1250.,150.};//{1500.,100.};//{1250.,150.};
    double trigEffParErrorsRot[] = {100.,50.};//{1250.,150.};//{100.,50.};
    double trigEffParsCharge[] = {1000.,100.};
    double trigEffParErrorsCharge[] = {100.,50.};
    double trigEffParsScint[] = {1000.,200.};
    double trigEffParErrorsScint[] = {100.,100.};
    double *trigEffPars = trigEffParsRot;
    double *trigEffParErrors = trigEffParErrorsRot;
    if(fCalibrationChannel == "Ionization")
    {
      trigEffPars = trigEffParsCharge;
      trigEffParErrors = trigEffParErrorsCharge;
    }      
    else if(fCalibrationChannel == "Scintillation")
    {
      trigEffPars = trigEffParsScint;
      trigEffParErrors = trigEffParErrorsScint;
    }      
    trigEffFunc->SetParameters(trigEffPars);
    trigEffFunc->SetParErrors(trigEffParErrors);
    //trigEffFunc->SetParLimits(0,1,5000);
    //trigEffFunc->SetParLimits(1,1,500);
    energyFitter.SetFunction("trigeff",trigEffFunc);
    isSet = true;
  }

  return isSet;  
}

void EXOEnergyCalibrator::SetInitialCalibPars(double par0, double par1)
{
  fCalibFitPars[0] = par0;
  fCalibFitPars[1] = par1;
}

void EXOEnergyCalibrator::SetInitialResPars(double par0, double par1)
{
  fResFitPars[0] = par0;
  fResFitPars[1] = par1;
}

void EXOEnergyCalibrator::SetHistoRanges(double energyLow_ss, double energyHigh_ss, double energyLow_ms, double energyHigh_ms)
{
  int nBins_ss = int((energyHigh_ss - energyLow_ss)/5.);
  int nBins_ms = int((energyHigh_ms - energyLow_ms)/5.);

  fNbins[0] = 0;
  fNbins[1] = nBins_ss;
  fNbins[2] = nBins_ms;

  fLowEnergy[0] = 0;
  fLowEnergy[1] = energyLow_ss;
  fLowEnergy[2] = energyLow_ms;

  fUpEnergy[0] = 0;
  fUpEnergy[1] = energyHigh_ss;
  fUpEnergy[2] = energyHigh_ms;
}

bool EXOEnergyCalibrator::SetHisto(const char* histoName, EXOEnergyMCBasedFit1D& energyFitter, const EXOSourceRunsPolishedInfo& runsInfo, const int multInt, const double weight)
{
  if(weight == 0.0)
  {
    std::cout << "Histogram " << histoName << " has weight " << weight << ", not setting to fit...\n";
    return true;
  }
  
  std::string multStr = (multInt == 1) ? "ss" : "ms";
  
  const std::vector<std::string> dataIds = runsInfo.GetListOf("DataId");
  const std::vector<std::string> dataMultWildCards = runsInfo.GetListOf("DataMultiplicityWildCard");
  const std::vector<std::string> sourceNames = runsInfo.GetListOf("SourceName");

  bool hasCs137 = false;
  std::vector<TString> fitHistos;
  size_t n = dataIds.size();
  for(size_t i = 0; i < n; i++)
  {
    TString dataId = dataIds.at(i);
    dataId = dataId.ReplaceAll(dataMultWildCards.at(i).c_str(),multStr.c_str());
    fitHistos.push_back(dataId.Data());
    if(sourceNames.at(i) == "Cs-137")
      hasCs137 = true;
  }

  bool hasWeek5or6or7 = false;
  const std::vector<std::string> weeks = runsInfo.GetListOf("WeekIndex");
  if(std::find(weeks.begin(),weeks.end(),"005") != weeks.end()) hasWeek5or6or7 = true;
  else if(std::find(weeks.begin(),weeks.end(),"006") != weeks.end()) hasWeek5or6or7 = true;
  else if(std::find(weeks.begin(),weeks.end(),"007") != weeks.end()) hasWeek5or6or7 = true;
  
  AdjustHistoRanges(hasCs137, hasWeek5or6or7);

  energyFitter.SetDataHisto(histoName,"",fitHistos,fNbins[multInt],fLowEnergy[multInt],fUpEnergy[multInt],weight);

  return true;
}

void EXOEnergyCalibrator::AdjustHistoRanges(bool incCs137, bool hasWeek5or6or7)
{

  if(fCalibrationChannel == "Rotated")
  {  
    if(fFitCalibrationType == "campaigns" || fFitCalibrationType == "noise" || fFitCalibrationType == "spatial-distribution")
    {
      fNbins[1] = 410; fNbins[2] = 800;//fNbins[2] = 800;//fNbins[1] = 680; fNbins[2] = 800;//fNbins[2] = 804; //fNbins[2] = 800;
      fLowEnergy[1] = 950; fLowEnergy[2] = 0;//fLowEnergy[2] = 0;//fLowEnergy[1] = 600; fLowEnergy[2] = 0;//fLowEnergy[2] = 980;//fLowEnergy[2] = 0;
      fUpEnergy[1] = 3000; fUpEnergy[2] = 4000;//fUpEnergy[2] = 4000;//fUpEnergy[2] = 5000;//fUpEnergy[2] = 4000;

      fNbins[1] = 460; fNbins[2] = 560;//fNbins[2] = 800;//fNbins[1] = 680; fNbins[2] = 800;//fNbins[2] = 804; //fNbins[2] = 800;
      fLowEnergy[1] = 700; fLowEnergy[2] = 700;//fLowEnergy[2] = 0;//fLowEnergy[1] = 600; fLowEnergy[2] = 0;//fLowEnergy[2] = 980;//fLowEnergy[2] = 0;
      fUpEnergy[1] = 3000; fUpEnergy[2] = 3500;//fUpEnergy[2] = 4000;//fUpEnergy[2] = 5000;//fUpEnergy[2] = 4000;

      fNbins[1] = 420; fNbins[2] = 520;//fNbins[2] = 800;//fNbins[1] = 680; fNbins[2] = 800;//fNbins[2] = 804; //fNbins[2] = 800;
      fLowEnergy[1] = 900; fLowEnergy[2] = 900;//fLowEnergy[2] = 0;//fLowEnergy[1] = 600; fLowEnergy[2] = 0;//fLowEnergy[2] = 980;//fLowEnergy[2] = 0;
      fUpEnergy[1] = 3000; fUpEnergy[2] = 3500;//fUpEnergy[2] = 4000;//fUpEnergy[2] = 5000;//fUpEnergy[2] = 4000;

    }
  
    if(incCs137 && fFitCalibrationType == "weekly")
    {
      //if (fLowEnergy[1] > 800.)
      //fLowEnergy[1] = 800.; fUpEnergy[1] = 4000.; fNbins[1] = 640;
      //fLowEnergy[2] = 0.; fUpEnergy[2] = 5000.; fNbins[2] = 1000; //ms real noise
      //fLowEnergy[2] = 800.; fUpEnergy[2] = 4000.; fNbins[2] = 840; //ms white noise
    }
  
    if(hasWeek5or6or7)
    {
      if(fFitCalibrationType == "weekly")
      {
        fLowEnergy[2] = 1300.; fUpEnergy[2] = 3000.; fNbins[2] = 340;//fUpEnergy[2] = 2000.; fNbins[2] = 140;
      }
      else if(fFitCalibrationType == "campaigns")
      {
        //fLowEnergy[2] = 1000.; fUpEnergy[2] = 2000.; fNbins[2] = 200;
      }
    }
  }

  if(fCalibrationChannel == "Ionization")
  {
      fNbins[1] = 640; fLowEnergy[1] = 800.; fUpEnergy[1] = 4000.; 
      fNbins[2] = 880; fLowEnergy[2] = 600.; fUpEnergy[2] = 5000.;
      if(hasWeek5or6or7)
      {
        if(fFitCalibrationType == "weekly")
        {
          fNbins[2] = 320; fLowEnergy[2] = 0.; fUpEnergy[2] = 1600.; //fUpEnergy[2] = 2000.; fNbins[2] = 140;
        }
      }

      if(fFitCalibrationType == "campaigns")
      {
        fNbins[1] = 680; fLowEnergy[1] = 600; fUpEnergy[1] = 4000;
        fNbins[2] = 800; fLowEnergy[2] = 0; fUpEnergy[2] = 4000;
      }
  }

    if(fCalibrationChannel == "Scintillation")
  {
    fNbins[1] = 800; fLowEnergy[1] = 2000.; fUpEnergy[1] = 10000.; 
    fNbins[2] = 1300; fLowEnergy[2] = 2000.; fUpEnergy[2] = 15000.;
    
    if(hasWeek5or6or7)
    {
      if(fFitCalibrationType == "weekly")
      {
        fNbins[2] = 600; fLowEnergy[2] = 0.; fUpEnergy[2] = 3000.; //fUpEnergy[2] = 2000.; fNbins[2] = 140;
      }
    }
    
    if(fFitCalibrationType == "campaigns")
    {
      fNbins[1] = 672; fLowEnergy[1] = 640; fUpEnergy[1] = 4000;
      fNbins[2] = 800; fLowEnergy[2] = 0; fUpEnergy[2] = 4000;
    }
      
  }

}

bool EXOEnergyCalibrator::SetWeightedHisto(const char* histoName, EXOEnergyMCBasedFit1D& energyFitter, const EXOSourceRunsPolishedInfo& runsInfo, const int multInt, const std::string keyDivision, const std::string keyWeight)
{
  if(keyDivision == "" || keyWeight == "")
    return SetHisto(histoName, energyFitter, runsInfo, multInt);

  const std::vector<std::string> divisions = runsInfo.GetListOf(keyDivision.c_str());
  const std::vector<std::string> weights = runsInfo.GetListOf(keyWeight.c_str());
 
  if(divisions.empty() || (divisions.size() != weights.size()))
  {
    std::cerr << "Cannot set weighted histograms, incompatible number of divisions and weights!\n";

    for(std::vector<std::string>::const_iterator division = divisions.begin(); division != divisions.end(); division++)
      std::cout << "Division: " << *division << std::endl;
    for(std::vector<std::string>::const_iterator weight = weights.begin(); weight != weights.end(); weight++)
      std::cout << "Weight: " << *weight << std::endl;
    
    return false;
  }

  std::map<std::string, double> histoWeights;
  size_t n = divisions.size();
  for(size_t i = 0; i < n; i++)
    histoWeights[divisions.at(i)] = atof(weights.at(i).c_str());

  std::string origHistoName = histoName;
  bool allOk = true;
  for(std::map<std::string, double>::iterator histoWeight = histoWeights.begin(); histoWeight != histoWeights.end(); histoWeight++)
  {
    EXOSourceRunsPolishedInfo selectedRunsInfo = runsInfo;
    selectedRunsInfo.CutExact(keyDivision,histoWeight->first);
    std::cout << "Cut on " << keyDivision << " = " << histoWeight->first << " at " << histoWeight->second << std::endl;

    std::string divHistoName = Form("%s_%s_%s",origHistoName.c_str(),keyDivision.c_str(),histoWeight->first.c_str());
    std::cout << "Histo names: " << origHistoName.c_str() << " " << keyDivision.c_str() << " " << histoWeight->first.c_str() << std::endl;
    std::cout << divHistoName.c_str() << std::endl;
    if(!SetHisto(divHistoName.c_str(), energyFitter, selectedRunsInfo, multInt, histoWeight->second))
      allOk = false;
  }

  std::cout << "All ok? " << allOk << std::endl;
  return allOk;  
}

bool EXOEnergyCalibrator::AddData(EXOEnergyMCBasedFit1D& energyFitter, const EXOSourceRunsPolishedInfo& runsInfo, const int multInt)
{
  std::string multStr = (multInt == 1) ? "ss" : "ms";

  const std::vector<std::string> inputFileNames = runsInfo.GetListOf("DataSelectionFileName");
  const std::vector<std::string> mcIds = runsInfo.GetListOf("MCId");
  const std::vector<std::string> mcMultWildCards = runsInfo.GetListOf("MCMultiplicityWildCard");
  const std::vector<std::string> dataIds = runsInfo.GetListOf("DataId");
  const std::vector<std::string> dataMultWildCards = runsInfo.GetListOf("DataMultiplicityWildCard");
  const std::vector<std::string> weeks = runsInfo.GetListOf("WeekIndex");
  const std::vector<std::string> startTimes = runsInfo.GetListOf("StartTimeSec");
  const std::vector<std::string> prescales = runsInfo.GetListOf("TriggerPrescale");
  

  if(inputFileNames.empty())
  {
    std::cout << "Cannot AddData, incompatible number files!\n";
    return false;
  }

  const std::vector<std::string> sourceZs = runsInfo.GetListOf("SourceZ");
  const std::vector<std::string> sourceAs = runsInfo.GetListOf("SourceA");
  const std::vector<std::string> sourcePosXs = runsInfo.GetListOf("SourcePositionX");
  const std::vector<std::string> sourcePosYs = runsInfo.GetListOf("SourcePositionY");
  const std::vector<std::string> sourcePosZs = runsInfo.GetListOf("SourcePositionZ");  
  
  bool allOk = true;
  std::map<int, double> weekAngle;
  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {   
    TString mcId = mcIds.at(i);
    mcId = mcId.ReplaceAll(mcMultWildCards.at(i).c_str(),multStr.c_str());

    TString inputFileName = inputFileNames.at(i);
    //inputFileName = inputFileName.ReplaceAll(dataMultWildCards.at(i).c_str(),multStr.c_str());

    TFile inputFile(inputFileName.Data(),"READ");
    if(inputFile.IsZombie())
    {
      std::cerr << "Cannot open file " << inputFileName.Data() << std::endl;
      return false;
    }

    /*
    TTree *tree = (TTree*) inputFile.Get("dataTree");
    if(!tree)
    {
      std::cerr << "Cannot get dataTree in file " << inputFileName.Data() << std::endl;
      return false;
    }

    tree->SetEstimate(tree->GetEntries()+1);
    tree->Draw("e_charge:e_scint","","para goff");
    std::vector<double> energy(tree->GetSelectedRows());
    */

    TEntryList *el = (TEntryList*) inputFile.Get(Form("EventList_%s",multStr.c_str()));
    if(!el)
    {
      std::cerr << "Cannot get data selection in file " << inputFileName.Data() << std::endl;
      return false;
    }
    
    TChain chain(el->GetTreeName());
    chain.Add(el->GetFileName());
    chain.SetEstimate(chain.GetEntries()+1);
    chain.SetEntryList(el);

    if (!fIsCalibrated) {chain.Draw("e_charge:e_scint","","goff");}
    else {chain.Draw(Form("energy_%s",multStr.c_str()),"","goff");}
 
    std::vector<double> energy(chain.GetSelectedRows());
    
    if(fCalibrationChannel == "Rotated")
    {
      int week = atoi(weeks.at(i).c_str());
      if(!weekAngle.count(week))
      {
        weekAngle[week] = GetWeekAngle(atol(startTimes.at(i).c_str()),multInt,week);
      }   
      
      //std::vector<double> e_rotated(chain.GetSelectedRows());
      for(size_t j = 0; j < energy.size(); j++)
      {
        if (!fIsCalibrated) {energy[j] = cos(weekAngle.at(week))*chain.GetV1()[j] + sin(weekAngle.at(week))*chain.GetV2()[j];}
        else {energy[j] = chain.GetV1()[j];}
        //std::cout << j << " " << week << " " << weekAngle.at(week) << " " << chain.GetV1()[j] << " " << chain.GetV2()[j] << std::endl;
      }
    }
    else if(fCalibrationChannel == "Ionization")
    {
      for(size_t j = 0; j < energy.size(); j++)
      {
        energy[j] = chain.GetV1()[j];
      }
    }
    else if(fCalibrationChannel == "Scintillation")
    {
      for(size_t j = 0; j < energy.size(); j++)
      {
        energy[j] = chain.GetV2()[j];
      }
    }

    if(fFitCalibrationType == "campaigns" || fFitCalibrationType == "noise" || fFitCalibrationType == "spatial-distribution")
    {
      for(size_t j = 0; j < energy.size(); j++)
        energy[j] = fWeeklyCalibFunc.at(Form("%s_%s",weeks.at(i).c_str(),multStr.c_str())).Eval(energy.at(j));
    }
        
    TString dataId = dataIds.at(i);
    dataId = dataId.ReplaceAll(dataMultWildCards.at(i).c_str(),multStr.c_str());
    if(fVerboseLevel > 0)
      std::cout << "Adding data: " << dataId.Data() << " associated to " << mcId.Data() << std::endl;
    if(!energyFitter.AddData(dataId.Data(),mcId.Data(),atoi(sourceZs.at(i).c_str()),atoi(sourceAs.at(i).c_str()),atof(sourcePosXs.at(i).c_str()),atof(sourcePosYs.at(i).c_str()),atof(sourcePosZs.at(i).c_str()),atoi(prescales.at(i).c_str()),&energy[0],(int)energy.size()))//&e_rotated[0],(int)e_rotated.size()))
      allOk = false;
  }

  return allOk;  
}


bool EXOEnergyCalibrator::AddMC(EXOEnergyMCBasedFit1D& energyFitter, const EXOSourceRunsPolishedInfo& runsInfo, const int multInt)
{
  std::string multStr = (multInt == 1) ? "ss" : "ms";
  
  const std::vector<std::string> inputFileNames = runsInfo.GetListOf("MCSelectionFileName");
  const std::vector<std::string> mcIds = runsInfo.GetListOf("MCId");
  const std::vector<std::string> multWildCards = runsInfo.GetListOf("MCMultiplicityWildCard");

  if(inputFileNames.empty() || (inputFileNames.size() != multWildCards.size() || inputFileNames.size() != mcIds.size()))
  {
    std::cout << "Cannot AddMC, incompatible number files!\n";
    return false;
  }

  const std::vector<std::string> sourceZs = runsInfo.GetListOf("SourceZ");
  const std::vector<std::string> sourceAs = runsInfo.GetListOf("SourceA");
  const std::vector<std::string> sourcePosXs = runsInfo.GetListOf("SourcePositionX");
  const std::vector<std::string> sourcePosYs = runsInfo.GetListOf("SourcePositionY");
  const std::vector<std::string> sourcePosZs = runsInfo.GetListOf("SourcePositionZ");  

  bool allOk = true;
  std::set<TString> processedMCs;
  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {
    TString mcId = mcIds.at(i);
    mcId = mcId.ReplaceAll(multWildCards.at(i),multStr.c_str());
    
    if(processedMCs.count(mcId.Data()))
      continue;
    
    if(fVerboseLevel > 0)
      std::cout << "Adding MC: " << mcId.Data() << std::endl;

    TString inputFileName = inputFileNames.at(i);
    inputFileName = inputFileName.ReplaceAll(multWildCards.at(i),multStr.c_str());

    TFile inputFile(inputFileName.Data(),"READ");
    if(inputFile.IsZombie())
    {
      std::cerr << "Cannot open file " << inputFileName.Data() << std::endl;
      return false;
    }
    TTree *tree = (TTree*) inputFile.Get("mcTree");
    if(!tree)
    {
      std::cerr << "Cannot get mcTree in file " << inputFileName.Data() << std::endl;
      return false;
    }
    tree->SetEstimate(tree->GetEntries()+1);
    tree->Draw("energy_mc:weight","","para goff");

    if(!energyFitter.AddMC(mcId.Data(),atoi(sourceZs.at(i).c_str()),atoi(sourceAs.at(i).c_str()),atof(sourcePosXs.at(i).c_str()),atof(sourcePosYs.at(i).c_str()),atof(sourcePosZs.at(i).c_str()),tree->GetV1(),tree->GetV2(),tree->GetSelectedRows()))
      allOk = false;

    processedMCs.insert(mcId.Data());
  }

  return allOk;

}

double EXOEnergyCalibrator::GetWeekAngle(long time, int multInt, int week)
{
  //EXOThoriumPeak* peak = GetCalibrationFor(EXOThoriumPeak,EXOThoriumPeakHandler,"2013-0nu-denoised",time);
  //return peak->GetRotationAngle(multInt);
  
  if (fUseAngleFromFile) {
      TTree *t = new TTree();
      t->ReadFile(fAngleFile);
      
      double file_angle;
      t->SetBranchAddress("angle",&file_angle);
      
      t->BuildIndex("week","mult");
      t->GetEntryWithIndex(week,multInt);
      
      return file_angle;
  }
  else {
      EXOEnergyMCBasedFit* mcBasedCalib = GetCalibrationFor(EXOEnergyMCBasedFit,EXOEnergyMCBasedFitHandler,fCalibFlavor,time);
      std::cout << "Getting angle.. " << fCalibFlavor << std::endl;
      return mcBasedCalib->GetParByName("Angle",multInt);
  }
}

