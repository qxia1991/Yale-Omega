#include "EXOEnergyUtils.hh"

ClassImp(EXOEnergyUtils)

std::string EXOEnergyUtils::fRatioFlavor = "vanilla";
std::string EXOEnergyUtils::fPeakFlavor = "vanilla";
std::string EXOEnergyUtils::fBiasFlavor = "vanilla";

double EXOEnergyUtils::fFVCut[3] = {162,10,182};

std::string EXOEnergyUtils::fCutType = "default";
std::string EXOEnergyUtils::fDiagonalCutDBFlavor = "2013-0nu-denoised";

bool EXOEnergyUtils::AddMCToFitterForMultiplicity(EXOEnergyMCBasedFit1D& energyFitter, const EXOSourceRunsPolishedInfo& runsInfo, const int multInt)
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
    
    //if(fVerboseLevel > 0)
    std::cout << "Adding MC: " << mcId.Data() << std::endl;

    TString inputFileName = inputFileNames.at(i);
    inputFileName = inputFileName.ReplaceAll(multWildCards.at(i),multStr.c_str());

    TFile inputFile(inputFileName.Data(),"READ");
    if(inputFile.IsZombie())
    {
      std::cerr << "Cannot open file " << inputFileName.Data() << std::endl;
      return false;
    }

    TEntryList *el = (TEntryList*) inputFile.Get(Form("EventList_%s",multStr.c_str()));
    if(!el)
    {
      std::cerr << "Cannot get MC selection in file " << inputFileName.Data() << std::endl;
      return false;
    }
    
    TChain chain(el->GetTreeName());
    chain.Add(el->GetFileName());
    chain.SetEntryList(el);

    /*
    TTree *tree = (TTree*) inputFile.Get("mcTree");
    if(!tree)
    {
      std::cerr << "Cannot get mcTree in file " << inputFileName.Data() << std::endl;
      return false;
    }
    tree->SetEstimate(tree->GetEntries()+1);
    tree->Draw("energy_mc:weight","","para goff");
    if(!energyFitter.AddMC(mcId.Data(),atoi(sourceZs.at(i).c_str()),atoi(sourceAs.at(i).c_str()),atof(sourcePosXs.at(i).c_str()),atof(sourcePosYs.at(i).c_str()),atof(sourcePosZs.at(i).c_str()),tree->GetV1(),tree->GetV2(),tree->GetSelectedRows()))
    */
    
    chain.SetEstimate(chain.GetEntries()+1);
    chain.Draw("energy_mc:weight","","goff");
    
    if(!energyFitter.AddMC(mcId.Data(),atoi(sourceZs.at(i).c_str()),atoi(sourceAs.at(i).c_str()),atof(sourcePosXs.at(i).c_str()),atof(sourcePosYs.at(i).c_str()),atof(sourcePosZs.at(i).c_str()),chain.GetV1(),chain.GetV2(),chain.GetSelectedRows()))
      allOk = false;

    processedMCs.insert(mcId.Data());
  }

  return allOk;
}

bool EXOEnergyUtils::CreateSourceDataTree(const EXOSourceRunsPolishedInfo& runsInfo, const char* setupFileName, const char* pythonName, bool applyZCorrection, bool isDenoised, const char* ZCorrectionDBFlavor, const char* calibDBFlavor0, const char* calibDBFlavor1, const char* calibDBFlavor2, const char* libEXOFittingScriptDir, const char* submitCommand, const char* scriptWildCard, bool submitJobs)
{
  const std::vector<std::string> inputFileNames = runsInfo.GetListOf("RecoFileName");
  const std::vector<std::string> outputFileNames = runsInfo.GetListOf("DataTreeFileName");

  if(inputFileNames.empty() || (inputFileNames.size() != outputFileNames.size()))
  {
    std::cerr << "Cannot create source data trees, incompatible number of input and output files!\n";
    return false;
  }

  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {
    // Create command file
    std::string cmdBody;

    std::string inDirFile = inputFileNames.at(i);
    size_t posSlash = inDirFile.rfind("/");
    std::string inRootFileName = inDirFile.substr(posSlash+1);
    inDirFile = inDirFile.substr(0,posSlash);

    std::string outDirFile = outputFileNames.at(i);
    posSlash = outDirFile.rfind("/");
    std::string outRootFileName = outDirFile.substr(posSlash+1);
    outDirFile = outDirFile.substr(0,posSlash);

    TString outFileName = outputFileNames.at(i);
    outFileName = outFileName.ReplaceAll(".root",".out");
    TString errFileName = outputFileNames.at(i);
    errFileName = errFileName.ReplaceAll(".root",".err");
    TString shellScriptFileName = outputFileNames.at(i);
    shellScriptFileName = shellScriptFileName.ReplaceAll(".root",".sh");

    TString commandFileName = outputFileNames.at(i);
    commandFileName = commandFileName.ReplaceAll(".root",".exo");

    cmdBody += Form("/tree/InputDirectory %s \n",inDirFile.c_str());
    cmdBody += Form("/tree/InputFilename %s \n",inRootFileName.c_str());

    cmdBody += Form("/tree/OutputDirectory %s \n",outDirFile.c_str());
    cmdBody += Form("/tree/OutputFilename %s \n",outRootFileName.c_str());

    cmdBody += Form("/tree/EnergyScintVariable %s \n",isDenoised ? "kScintDenoisedEnergy" : "kScintRawEnergy");
    cmdBody += Form("/tree/ApplyZcorrection %s \n",applyZCorrection ? "True" : "False");
    cmdBody += Form("/tree/EnergyZcorrectionDBflavor %s \n",ZCorrectionDBFlavor);

    cmdBody += Form("/tree/CalibrationDBflavor0 %s \n",calibDBFlavor0);
    cmdBody += Form("/tree/CalibrationDBflavor1 %s \n",calibDBFlavor1);
    cmdBody += Form("/tree/CalibrationDBflavor2 %s \n",calibDBFlavor2);

    cmdBody += Form("/tree/UseRandomTrigger true \n");
    cmdBody += Form("/tree/MaxDriftTime 107.0 \n");

    cmdBody += Form("/tree/UseMCBasedFitEnergyCalibration true \n");
    cmdBody += Form("/tree/CalibrateChargeClusters false \n");
    cmdBody += Form("/tree/CalibrateEventChargeEnergy false \n");
    cmdBody += Form("/tree/CalibrateEventLightEnergy false \n");

    // Write the command file
    std::ofstream cmdFile(commandFileName.Data());
    cmdFile << cmdBody.c_str();
    cmdFile.close();

    // Create shell script
    std::string shellScriptBody = Form("source %s;\ncd %s;\n%s EXOFitting.py -m tree -a build -c %s > %s 2> %s\n",setupFileName,libEXOFittingScriptDir,pythonName,commandFileName.Data(),outFileName.Data(),errFileName.Data());

    // Write shell script file
    std::ofstream shellScriptFile(shellScriptFileName.Data());
    shellScriptFile << shellScriptBody.c_str();
    shellScriptFile.close();
    
    TString permissionCommandStr = Form("chmod 755 %s",shellScriptFileName.Data());
    system(permissionCommandStr.Data());

    TString submitCommandStr = submitCommand;
    submitCommandStr = submitCommandStr.ReplaceAll(scriptWildCard,shellScriptFileName.Data());
    std::cout << submitCommandStr.Data() << std::endl;
    if(submitJobs)
      system(submitCommandStr.Data());
  }      
  
  return true;
}

bool EXOEnergyUtils::CutTreeForMCBasedFit(const EXOSourceRunsPolishedInfo& runsInfo, const char* setupFileName, const char* pythonName, const char* scriptDir, const char* submitCommand, const char* scriptWildCard, bool cutMC, bool cutData, bool submitJobs)
{
  EXOFiducialVolume::SetUserHexCut(fFVCut[0],fFVCut[1],fFVCut[2]);
  
  std::string shellScriptBodyPat = "source %s;\ncd %s;\n%s CutTree.py \'%s\' \'%s\' \'%s\' \'%s\' \'%s\' > %s 2>%s;\n";
  shellScriptBodyPat = Form(shellScriptBodyPat.c_str(),setupFileName,scriptDir,pythonName,"%s","%s","%s","%s","%s","%s","%s");

  bool allOk = true;

  if(cutMC)
    if(!CutTreeForMCBasedFitIn("MC",runsInfo,shellScriptBodyPat,submitCommand,scriptWildCard,submitJobs))
      allOk = false;

  if(cutData)
    if(!CutTreeForMCBasedFitIn("Data",runsInfo,shellScriptBodyPat,submitCommand,scriptWildCard,submitJobs))
      allOk = false;
  
  return allOk;
}

bool EXOEnergyUtils::CutSourceDataTree(const EXOSourceRunsPolishedInfo& runsInfo, const char* setupFileName, const char* pythonName, bool useRandomTrigger, const char* diagonalCutFlavor, double fiducialCut0, double fiducialCut1, double fiducialCut2, double fiducialCut3, const char* libEXOFittingScriptDir, const char* submitCommand, const char* scriptWildCard, bool isMC, bool submitJobs)
{
  TString typeInput = (isMC) ? "MCTreeFileName":"DataTreeFileName";
  TString typeOutput = (isMC) ? "MCSelectionFileName":"SelectionTreeFileName";
  const std::vector<std::string> inputFileNames = runsInfo.GetListOf(typeInput.Data());
  const std::vector<std::string> outputFileNames = runsInfo.GetListOf(typeOutput.Data());

  if(inputFileNames.empty() || (inputFileNames.size() != outputFileNames.size()))
  {
    std::cerr << "Cannot create source data trees, incompatible number of input and output files!\n";
    return false;
  }

  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {    
    // Create commands file
    std::string cmdBody;

    TString outFileName = outputFileNames.at(i);
    outFileName = outFileName.ReplaceAll(".root",".out");
    TString errFileName = outputFileNames.at(i);
    errFileName = errFileName.ReplaceAll(".root",".err");

    TString shellScriptFileName = outputFileNames.at(i);
    shellScriptFileName = shellScriptFileName.ReplaceAll(".root",".sh");

    std::string inDirFile = inputFileNames.at(i);
    size_t posSlash = inDirFile.rfind("/");
    std::string inRootFileName = inDirFile.substr(posSlash+1);
    inDirFile = inDirFile.substr(0,posSlash);

    std::string outDirFile = outputFileNames.at(i);
    posSlash = outDirFile.rfind("/");
    std::string outRootFileName = outDirFile.substr(posSlash+1);
    outDirFile = outDirFile.substr(0,posSlash);

    TString commandFileName = outputFileNames.at(i);
    commandFileName = commandFileName.ReplaceAll(".root",".exo");

    cmdBody += Form("/tree/InputDirectory %s \n",inDirFile.c_str());
    cmdBody += Form("/tree/InputFilename %s \n",inRootFileName.c_str());

    cmdBody += Form("/tree/OutputDirectory %s \n",outDirFile.c_str());
    cmdBody += Form("/tree/OutputFilename %s \n",outRootFileName.c_str());

    if (isMC) {cmdBody += "/tree/IsData false \n";}

    cmdBody += "/tree/OnlyPositiveEnergy true \n";

    cmdBody += Form("/tree/UseRandomTrigger %s \n",useRandomTrigger ? "true" : "false");
    cmdBody += Form("/tree/DiagonalCutDBFlavor %s \n",diagonalCutFlavor);

    cmdBody += Form("/tree/CutFV0 %f \n",fiducialCut0);
    cmdBody += Form("/tree/CutFV1 %f \n",fiducialCut1);
    cmdBody += Form("/tree/CutFV2 %f \n",fiducialCut2);
    cmdBody += Form("/tree/CutFV3 %f \n",fiducialCut3);

    // Write the command file
    std::ofstream cmdFile(commandFileName.Data());
    cmdFile << cmdBody.c_str();

    // Create shell script
    std::string shellScriptBody = Form("source %s;\ncd %s;\n%s EXOFitting.py -m tree -a cut -c %s > %s 2> %s\n",setupFileName,libEXOFittingScriptDir,pythonName,commandFileName.Data(),outFileName.Data(),errFileName.Data());

    // Write shell script file
    std::ofstream shellScriptFile(shellScriptFileName.Data());
    shellScriptFile << shellScriptBody.c_str();
    shellScriptFile.close();
    
    TString permissionCommandStr = Form("chmod 755 %s",shellScriptFileName.Data());
    system(permissionCommandStr.Data());

    TString submitCommandStr = submitCommand;
    submitCommandStr = submitCommandStr.ReplaceAll(scriptWildCard,shellScriptFileName.Data());
    std::cout << submitCommandStr.Data() << std::endl;
    if(submitJobs)
      system(submitCommandStr.Data());
  }      
  
  return true;
}

bool EXOEnergyUtils::FriendSourceDataTree(const EXOSourceRunsPolishedInfo& runsInfo, const char* setupFileName, const char* pythonName, const char* prepDir, const char* prepName, const char* weightDir, const char* factoryName, const char* libEXOFittingScriptDir, const char* submitCommand, const char* scriptWildCard, bool isMC, bool submitJobs)
{
  TString typeInput = (isMC) ? "MCSelectionFileName":"SelectionTreeFileName";
  TString typeOutput = (isMC) ? "MCFriendFileName":"FriendTreeFileName";
  const std::vector<std::string> inputFileNames = runsInfo.GetListOf(typeInput.Data());
  const std::vector<std::string> outputFileNames = runsInfo.GetListOf(typeOutput.Data());

  if(inputFileNames.empty() || (inputFileNames.size() != outputFileNames.size()))
  {
    std::cerr << "Cannot create source data trees, incompatible number of input and output files!\n";
    return false;
  }

  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {    
    // Create commands file
    std::string cmdBody;

    TString outFileName = outputFileNames.at(i);
    outFileName = outFileName.ReplaceAll(".root",".out");
    TString errFileName = outputFileNames.at(i);
    errFileName = errFileName.ReplaceAll(".root",".err");

    TString shellScriptFileName = outputFileNames.at(i);
    shellScriptFileName = shellScriptFileName.ReplaceAll(".root",".sh");

    std::string inDirFile = inputFileNames.at(i);
    size_t posSlash = inDirFile.rfind("/");
    std::string inRootFileName = inDirFile.substr(posSlash+1);
    inDirFile = inDirFile.substr(0,posSlash);

    std::string outDirFile = outputFileNames.at(i);
    posSlash = outDirFile.rfind("/");
    std::string outRootFileName = outDirFile.substr(posSlash+1);
    outDirFile = outDirFile.substr(0,posSlash);

    TString commandFileName = outputFileNames.at(i);
    commandFileName = commandFileName.ReplaceAll(".root",".exo");

    cmdBody += Form("/mldis/InputDirectory %s \n",prepDir);
    cmdBody += Form("/mldis/InputFilename %s \n",prepName);

    cmdBody += "/mldis/OnlyApplyToCut true \n";
    cmdBody += "/mldis/FriendAdvVars true \n";

    cmdBody += Form("/mldis/WeightsDirectory %s \n",weightDir);
    cmdBody += Form("/mldis/AddMethod BDT \n");
    cmdBody += Form("/mldis/FactoryName %s \n",factoryName);
    
    cmdBody += Form("/mldis/DiscTreeDir %s \n",outDirFile.c_str());
    cmdBody += Form("/mldis/DiscTreeSuffix disc \n");

    cmdBody += Form("/mldis/CutTreeDir %s \n",inDirFile.c_str());
    if (isMC) {cmdBody += "/mldis/IsData false \n";}
    else {cmdBody += "/mldis/IsData true \n";}
    
    cmdBody += Form("/mldis/AddData %s \n",inRootFileName.c_str());

    // Write the command file
    std::ofstream cmdFile(commandFileName.Data());
    cmdFile << cmdBody.c_str();

    // Create shell script
    std::string shellScriptBody = Form("source %s;\ncd %s;\n%s EXOFitting.py -m mldis -a apply -c %s > %s 2> %s\n",setupFileName,libEXOFittingScriptDir,pythonName,commandFileName.Data(),outFileName.Data(),errFileName.Data());

    // Write shell script file
    std::ofstream shellScriptFile(shellScriptFileName.Data());
    shellScriptFile << shellScriptBody.c_str();
    shellScriptFile.close();
    
    TString permissionCommandStr = Form("chmod 755 %s",shellScriptFileName.Data());
    system(permissionCommandStr.Data());

    TString submitCommandStr = submitCommand;
    submitCommandStr = submitCommandStr.ReplaceAll(scriptWildCard,shellScriptFileName.Data());
    std::cout << submitCommandStr.Data() << std::endl;
    if(submitJobs)
      system(submitCommandStr.Data());
  }      
  
  return true;
}


bool EXOEnergyUtils::CutTreeForMCBasedFitIn(TString type, const EXOSourceRunsPolishedInfo& runsInfo, std::string shellScriptBodyPat, const char* submitCommand, const char* scriptWildCard, bool submitJobs)
{
  std::vector<TString> types;
  types.push_back("Data");
  types.push_back("MC");

  if(std::find(types.begin(),types.end(),type) == types.end())
  {
    std::cerr << "Wrong type of data, it must be either data or MC!\n";
    return false;
  }
  
  const std::vector<std::string> inputFileNames = runsInfo.GetListOf(Form("%sTreeFileName",type.Data()));
  const std::vector<std::string> outputFileNames = runsInfo.GetListOf(Form("%sSelectionFileName",type.Data()));
  const std::vector<std::string> multWildCards = runsInfo.GetListOf(Form("%sMultiplicityWildCard",type.Data()));

  if(inputFileNames.empty() || (inputFileNames.size() != outputFileNames.size()))
  {
    std::cout << "Cannot cuttrees, incompatible number of input and output files!\n";
    return false;
  }

  type.ToLower();
  
  std::vector<std::string> cut(3);
  cut[1] = (type == "data") ? GetDefaultCut(1,1) : GetDefaultCut(0,1);
  cut[2] = (type == "data") ? GetDefaultCut(1,2) : GetDefaultCut(0,2);

  std::string treeName = Form("%sTree",type.Data());
  
  std::set<std::string> processedFiles;
  std::vector<std::string> notFiles;
  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {
    std::string inputFileName = inputFileNames.at(i);
    if(processedFiles.count(inputFileName.c_str()))
      continue;
    processedFiles.insert(inputFileName.c_str());
    
    struct stat buffer;
    bool fileExist = (stat(inputFileNames.at(i).c_str(),&buffer) == 0);
    if(!fileExist)
    {
      std::cout << inputFileNames.at(i) << " does not exist!!!\n";
      notFiles.push_back(inputFileNames.at(i));
      continue;
    }    

    for(int multInt = 1; multInt <= 2; multInt++)
    {
      std::string multStr = (multInt == 1) ? "ss" : "ms";

      TString rootFileName = outputFileNames.at(i);
      rootFileName = rootFileName.ReplaceAll(multWildCards.at(i),multStr.c_str());
      
      TString outFileName = outputFileNames.at(i);
      outFileName = outFileName.ReplaceAll(".root",".out");
      outFileName = outFileName.ReplaceAll(multWildCards.at(i),multStr.c_str());
      TString errFileName = outputFileNames.at(i);
      errFileName = errFileName.ReplaceAll(".root",".err");
      errFileName = errFileName.ReplaceAll(multWildCards.at(i),multStr.c_str());

      std::string shellScriptBody = Form(shellScriptBodyPat.c_str(),inputFileName.c_str(),treeName.c_str(),rootFileName.Data(),cut.at(multInt).c_str(),fDiagonalCutDBFlavor.c_str(),outFileName.Data(),errFileName.Data());
            
      TString shellScriptFileName = outputFileNames.at(i);
      shellScriptFileName = shellScriptFileName.ReplaceAll(".root",".sh");
      shellScriptFileName = shellScriptFileName.ReplaceAll(multWildCards.at(i),multStr.c_str());
    
      std::ofstream shellScriptFile(shellScriptFileName.Data());
      shellScriptFile << shellScriptBody.c_str();
      shellScriptFile.close();

      TString permissionCommandStr = Form("chmod 755 %s",shellScriptFileName.Data());
      system(permissionCommandStr.Data());

      TString submitCommandStr = submitCommand;
      submitCommandStr = submitCommandStr.ReplaceAll(scriptWildCard,shellScriptFileName.Data());
      std::cout << submitCommandStr.Data() << std::endl;
      if(submitJobs)
        system(submitCommandStr.Data());
      //sleep(2);
    }
    
  }

  std::cout << "Files that does not exist:\n";
  for(std::vector<std::string>::iterator notFile = notFiles.begin(); notFile != notFiles.end(); notFile++)
    std::cout << *notFile << std::endl;

  return true;    
}



const std::string EXOEnergyUtils::GetDefaultCut(bool isDataOrMC, int multInt)
{
  /*std::string multStr = (multInt > 1) ? "ms" : "ss";
  std::string multCut = (multInt > 1) ? "multiplicity > 1.5" : "multiplicity < 1.5";
  
  EXOFitter exoFitter;
  exoFitter.SetApplyVetoes(false);
  exoFitter.SetRandomTrigger(true);

  std::string addCut = (isDataOrMC) ? Form("e_scint > 0. && e_charge >0. && energy_%s > 0.",multStr.c_str()) : "energy_mc > 0";

  std::string fitCut = "";
  REQUIRES UPDATE TO NEW EXO FITTER PREPROCESSEDTREEHANDLER!!!!!!!
  if(fCutType == "default")
    fitCut = (isDataOrMC) ? exoFitter.GetDefaultDataCut() : exoFitter.GetDefaultMCCut();
  else if(fCutType == "cosmogenics")
    fitCut = (isDataOrMC) ? exoFitter.GetDefaultDataVetoLikeCut() : exoFitter.GetDefaultMCVetoLikeCut();
  
  std::string finalCut = fitCut + Form(" && %s && %s", multCut.c_str(),addCut.c_str());

  return finalCut;*/

  isDataOrMC = false;
  multInt = 1;

  return "";
}

const std::string EXOEnergyUtils::GetSourceName(int sourceZ, int sourceA)
{
  std::string element;
  if(sourceZ == 27)
    element = "Co";
  if(sourceZ == 55)
    element = "Cs";
  if(sourceZ == 88)
    element = "Ra";
  if(sourceZ == 90)
    element = "Th";
    
  return Form("%s-%d",element.c_str(),sourceA);
}

int EXOEnergyUtils::GetAtomicNumber(const char* sourceName)
{
  TString sourceNameStr(sourceName);
  sourceNameStr.ToLower();
  if(sourceNameStr.Contains("co"))
    return 27;
  if(sourceNameStr.Contains("cs"))
    return 55;
  if(sourceNameStr.Contains("ra"))
    return 88;
  if(sourceNameStr.Contains("th"))
    return 90;

  return 0;
}

int EXOEnergyUtils::GetIsotopeNumber(const char* sourceName)
{
  std::string sourceNameStr(sourceName);

  size_t dashPos = sourceNameStr.find("-");

  std::string isotopeNumberStr = sourceNameStr.substr(dashPos+1);

  return atoi(isotopeNumberStr.c_str());
}


const std::vector<int>* EXOEnergyUtils::GetAllLJRunsForSource()
{
  std::vector<int>* runsFound = 0;
  for(int week = 0; week < 150; week++)
  {
    const std::vector<int>* runsCs = GetLJSourceRuns(week,"Cs-137");
    const std::vector<int>* runsCo = GetLJSourceRuns(week,"Co-60");
    const std::vector<int>* runsTh = GetLJSourceRuns(week,"Th-228");

    if(runsCs || runsCo || runsTh)
    {
      if(!runsFound)
        runsFound = new std::vector<int>();
      if(runsCs)
        for(std::vector<int>::const_iterator run = runsCs->begin(); run != runsCs->end(); run++)
          runsFound->push_back(*run);
      if(runsCo)
        for(std::vector<int>::const_iterator run = runsCo->begin(); run != runsCo->end(); run++)
          runsFound->push_back(*run);
      if(runsTh)
        for(std::vector<int>::const_iterator run = runsTh->begin(); run != runsTh->end(); run++)
          runsFound->push_back(*run);
    }
  }
  return runsFound;
}


const std::vector<int>* EXOEnergyUtils::GetLJSourceRuns(int week, int sourceZ, int sourceA)
{
  return GetLJSourceRuns(week, GetSourceName(sourceZ,sourceA).c_str());
}

const std::vector<int>* EXOEnergyUtils::GetLJSourceRuns(int week, const char* sourceName)
{
  std::string sourceNameStr(sourceName);
  std::vector<int>* runsFound = 0;

  std::ifstream runsListFile;
  runsListFile.open("../data/LJRunList.txt");
  std::string fileLine;
  
  while(std::getline(runsListFile,fileLine))
  {
    std::stringstream inFileLine (fileLine);
    std::vector<std::string> words;
    std::string word;
    while(inFileLine >> word)
    {
      words.push_back(word);
    }
    if(words.size() < 3)
      continue;
    if(sourceNameStr != words[0])
        continue;
    int runsWeek = atoi(words[1].c_str());
    if(runsWeek != week)
      continue;
    runsFound = new std::vector<int>();
    for(size_t i = 2; i < words.size() && words[i] != "from"; i++)
    {
      runsFound->push_back(atoi(words[i].c_str()));
    }    
  }

  return runsFound;
}

