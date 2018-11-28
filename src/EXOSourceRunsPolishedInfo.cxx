#include "EXOSourceRunsPolishedInfo.hh"

ClassImp(EXOSourceRunsPolishedInfo)

EXOSourceRunsPolishedInfo::EXOSourceRunsPolishedInfo(const char* sourceRunsInfoFileName)
{
  ReadSourceRunsInfoFromFile(sourceRunsInfoFileName);
  ClearExceptRuns();
}

EXOSourceRunsPolishedInfo::~EXOSourceRunsPolishedInfo()
{
}

bool EXOSourceRunsPolishedInfo::ReadSourceRunsInfoFromFile(const char* sourceRunsInfoFileName)
{
  std::cout << "Reading source run info from file: " << sourceRunsInfoFileName << std::endl;
  std::ifstream infoFile(sourceRunsInfoFileName);
  
  if(!infoFile)
  {
    std::cout << "Problem reading file given for source data information" << std::endl;// << sourceRunsInfoFileName << std::endl;
    std::cout << "*** Please set file name using SetSourceRunInfoFile ***\n";    
    return false;
  }

  std::string fileLine;

  Int_t currentRun = 0;
  std::vector<EXOMetadata> currentVecMD;
  while(std::getline(infoFile,fileLine))
  {
    if(fileLine.length() <= 0)
      continue;
    
    size_t colonPos = fileLine.find(":");

    if(colonPos == std::string::npos)
      continue;
    
    std::string key = fileLine.substr(0,colonPos-1);
    std::string value = fileLine.substr(colonPos+2);

    EXOMetadata md(key,value);

    if(key == "EndOfSourceRunInfo")
    {
      std::vector<EXOMetadata> vecMD = currentVecMD;
      fAllMD.insert(std::pair<Int_t, std::vector<EXOMetadata> >(currentRun,vecMD));
      currentVecMD.clear();
      continue;
    }

    if(key == "RunNumber")
      currentRun = md.AsInt();

    currentVecMD.push_back(md);
  }
  
  fSourceRunsInfoFileName = sourceRunsInfoFileName;

  return true;    
}

bool EXOSourceRunsPolishedInfo::SetSourceRunsInfoFile(const char* sourceRunsInfoFileName)
{
  return ReadSourceRunsInfoFromFile(sourceRunsInfoFileName);
}

const char* EXOSourceRunsPolishedInfo::GetSourceRunsInfoFileName()
{
  return fSourceRunsInfoFileName.Data();
}

void EXOSourceRunsPolishedInfo::Print(Int_t runNo) const
{
  if(fAllMD.find(runNo) != fAllMD.end())
  {
    std::cout << "Print info for run# " << runNo << std::endl;
    const std::vector<EXOMetadata> & vecMD = fAllMD.at(runNo);
    for(std::vector<EXOMetadata>::const_iterator md  = vecMD.begin(); md != vecMD.end(); md++)
      md->Print();
  }       
  else
  {
    for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
    {
      std::cout << "Print info for run# " << runMD->first << std::endl;
      const std::vector<EXOMetadata> & vecMD = runMD->second;
      
      for(std::vector<EXOMetadata>::const_iterator md  = vecMD.begin(); md != vecMD.end(); md++)
      {
        md->Print();
      }
    }
  }
  return;
}

void EXOSourceRunsPolishedInfo::PrintRunList(Int_t runNo, TString outputRunsInfoFileName) const
{
  std::ofstream fout;
  if(outputRunsInfoFileName != "")
    fout.open(outputRunsInfoFileName);

  if(fAllMD.find(runNo) != fAllMD.end())
  {
    const std::vector<EXOMetadata> & vecMD = fAllMD.at(runNo);
    for(std::vector<EXOMetadata>::const_iterator md  = vecMD.begin(); md != vecMD.end(); md++)
      (fout.is_open()?fout:std::cout) << md->GetKey() << " : " << md->AsString() << std::endl;
    (fout.is_open()?fout:std::cout) << "EndOfSourceRunInfo : ------------------------------------------------------" << std::endl;
  }       
  else
  {
    for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
    {
      const std::vector<EXOMetadata> & vecMD = runMD->second;
      
      for(std::vector<EXOMetadata>::const_iterator md  = vecMD.begin(); md != vecMD.end(); md++)
        (fout.is_open()?fout:std::cout) << md->GetKey() << " : " << md->AsString() << std::endl;
      (fout.is_open()?fout:std::cout) << "EndOfSourceRunInfo : ------------------------------------------------------" << std::endl;
    }
  }
  if(fout.is_open()) fout.close();

  return;
}

size_t EXOSourceRunsPolishedInfo::GetNRuns() const
{
  return fAllMD.size();
}

const EXOMetadata* EXOSourceRunsPolishedInfo::FindMetadata(const std::string& key, const std::vector<EXOMetadata> & vecMD) const
{
  for(std::vector<EXOMetadata>::const_iterator md  = vecMD.begin(); md != vecMD.end(); md++)
  {
    if(md->GetKey() == key)
      return &(*md);
  }
  return 0;
}

const std::vector<std::string> EXOSourceRunsPolishedInfo::GetListOf(const char* key) const
{
  std::vector<std::string> vecKey;// = new std::vector<std::string>();
  for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    const std::vector<EXOMetadata> & vecMD = runMD->second;
    const EXOMetadata* md = FindMetadata(key,vecMD);
    if(md)
      vecKey.push_back(md->AsString());
  }
  
  return vecKey;
}

const std::set<std::string> EXOSourceRunsPolishedInfo::GetSetOf(const char* key) const
{
  std::vector<std::string> listKey = GetListOf(key);
  std::set<std::string> setKey(listKey.begin(),listKey.end());

  return setKey;
}

void EXOSourceRunsPolishedInfo::ClearExceptRuns()
{
  fForceKeep.clear();
  fForceDiscard.clear();
}

void EXOSourceRunsPolishedInfo::AddExactCondition(const std::string& key, const std::string& value, bool toKeepOrDiscard)
{
  for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    const std::vector<EXOMetadata> & vecMD = runMD->second;
    const EXOMetadata* md = FindMetadata(key,vecMD);
    if(md)
    {
      if(toKeepOrDiscard && md->AsString() == value)
        fForceKeep.push_back(runMD->first);
      else if(!toKeepOrDiscard && md->AsString() == value)
        fForceDiscard.push_back(runMD->first);
    }
  }
}

bool EXOSourceRunsPolishedInfo::CutDoubleComparison(const std::string& key, double value, bool greaterOrSmaller)
{
  std::vector<Int_t> cut;
  for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    const std::vector<EXOMetadata> & vecMD = runMD->second;
    const EXOMetadata* md = FindMetadata(key,vecMD);
    if(md)
    {
      //std::cout << "Run " << runMD->first << std::endl;
      if(std::find(fForceKeep.begin(),fForceKeep.end(),runMD->first) != fForceKeep.end())
      {
        //std::cout << " 1 " << std::endl;
        continue;
      }
      else if(std::find(fForceDiscard.begin(),fForceDiscard.end(),runMD->first) != fForceDiscard.end())
      {
        //std::cout << " 2 " << std::endl;
        cut.push_back(runMD->first);
      }
      else if(greaterOrSmaller && md->AsDouble() < value)
      {
        //std::cout << " 3 " << std::endl;
        cut.push_back(runMD->first);
      }
      else if(!greaterOrSmaller && md->AsDouble() > value)
      {
        //std::cout << " 4 " << std::endl;
        cut.push_back(runMD->first);
      }
    }
  }

  for(std::vector<Int_t>::iterator run = cut.begin(); run != cut.end(); run++)
    fAllMD.erase(*run);

  return true;  
}

bool EXOSourceRunsPolishedInfo::CutExact(const std::string& key, const std::string& value, bool toKeepOrDiscard)
{
  std::vector<Int_t> cut;
  for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    const std::vector<EXOMetadata> & vecMD = runMD->second;
    const EXOMetadata* md = FindMetadata(key,vecMD);
    if(md)
    {
      if(std::find(fForceKeep.begin(),fForceKeep.end(),runMD->first) != fForceKeep.end())
        continue;
      else if(std::find(fForceDiscard.begin(),fForceDiscard.end(),runMD->first) != fForceDiscard.end())
        cut.push_back(runMD->first);
      else if(toKeepOrDiscard && md->AsString() != value)
        cut.push_back(runMD->first);
      else if(!toKeepOrDiscard && md->AsString() == value)
        cut.push_back(runMD->first);
    }
  }
  
  for(std::vector<Int_t>::iterator run = cut.begin(); run != cut.end(); run++)
    fAllMD.erase(*run);

  return true;  
}

bool EXOSourceRunsPolishedInfo::CutExactList(const std::string& key, const std::vector<std::string>& values, bool toKeepOrDiscard)
{
  std::vector<Int_t> cut;
  for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    const std::vector<EXOMetadata> & vecMD = runMD->second;
    const EXOMetadata* md = FindMetadata(key,vecMD);
    if(md)
    {
      if(std::find(fForceKeep.begin(),fForceKeep.end(),runMD->first) != fForceKeep.end())
        continue;
      else if(std::find(fForceDiscard.begin(),fForceDiscard.end(),runMD->first) != fForceDiscard.end())
        cut.push_back(runMD->first);
      else if(toKeepOrDiscard && (std::find(values.begin(),values.end(),md->AsString()) == values.end()))
        cut.push_back(runMD->first);
      else if(!toKeepOrDiscard && (std::find(values.begin(),values.end(),md->AsString()) != values.end()))
        cut.push_back(runMD->first);
    }
  }
  
  for(std::vector<Int_t>::iterator run = cut.begin(); run != cut.end(); run++)
    fAllMD.erase(*run);

  return true;  
}

bool EXOSourceRunsPolishedInfo::SelectDefaultRuns()
{
  return CutDefaultRuns();
}

bool EXOSourceRunsPolishedInfo::CutDefaultRuns()
{
  //Cs at S2, no simulation...
  CutExact("RunNumber","2410",false);
  CutExact("RunNumber","2450",false);

  //Processing problem, should be fixed (was veto info)
  //CutExact("RunNumber","5822",false)

  // super short runs, no data
  CutExact("RunNumber","3538",false);
  CutExact("RunNumber","3539",false);
  //

  CutExact("RunNumber","6940",false); // Suspect
  CutExact("RunNumber","7062",false); // changed APD gain
  CutExact("RunNumber","7063",false); // changed APD gain
  CutExact("RunNumber","7064",false); // changed APD gain
  CutExact("RunNumber","7069",false); // changed APD gain
  CutExact("RunNumber","7070",false); // changed APD gain
  CutExact("RunNumber","7071",false); // changed APD gain
  CutExact("RunNumber","7074",false); // run at 12kV bias
  CutExact("RunNumber","7075",false); // run at 12kV bias
  CutExact("RunNumber","7103",false); // only run at 8kV for this week

  CutExact("SourcePositionS","0",false);
  CutExact("SourcePositionS","17",false);
  CutExact("ManualFlag","NONE");
  CutExact("Quality","BAD",false);
  AddExactCondition("SourceName","Cs-137",true);
  AddExactCondition("SourceName","Ra-226",true);
  AddExactCondition("SourceName","Co-60",true);
  CutDoubleComparison("Purity",1.0,true); // only check for fit failures
  CutDoubleComparison("Purity",9.9,false); // only check for fit failures
  ClearExceptRuns();

  return true;
}

bool EXOSourceRunsPolishedInfo::SelectMaxPurity(double value, bool greaterOrSmaller)
{
  std::vector<Int_t> cut;
  for(std::map<Int_t, std::vector<EXOMetadata> >::const_iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    const std::vector<EXOMetadata> & vecMD = runMD->second;
    const EXOMetadata* mdpur = FindMetadata("Purity",vecMD);
    const EXOMetadata* mdrun = FindMetadata("RunPurity",vecMD);
    if(not (mdpur && mdrun))
      return false;
    
    double maxpur = std::max(mdpur->AsDouble(),mdrun->AsDouble());
    if(std::find(fForceKeep.begin(),fForceKeep.end(),runMD->first) != fForceKeep.end())
      continue;
    else if(std::find(fForceDiscard.begin(),fForceDiscard.end(),runMD->first) != fForceDiscard.end())
      cut.push_back(runMD->first);
    else if(greaterOrSmaller && maxpur < value)
      cut.push_back(runMD->first);
    else if(!greaterOrSmaller && maxpur > value)
      cut.push_back(runMD->first);
  }

  for(std::vector<Int_t>::iterator run = cut.begin(); run != cut.end(); run++)
    fAllMD.erase(*run);

  return true;  
}


void EXOSourceRunsPolishedInfo::SetMetadata(const std::string& key, const std::string& patData)
{
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;
    TString metaPat = patData;
    for(std::vector<EXOMetadata>::iterator md = vecMD.begin(); md != vecMD.end(); md++)
    {
      metaPat.ReplaceAll(Form("[%s]",md->GetKey().c_str()),md->AsString().c_str());
    }

    EXOMetadata newMD(key,metaPat.Data());
    vecMD.push_back(newMD);
  }    
}

void EXOSourceRunsPolishedInfo::SetRecoFileNames(const char* patName, const char* runWildcard, const char* dirFiles)
{
  std::string key = "RecoFileName";

  TString dir = dirFiles;
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;

    TString fileName = patName;
    fileName = fileName.ReplaceAll(runWildcard,Form("%d",runMD->first));
    fileName = dir + "/" + fileName;
    
    EXOMetadata recoFileName(key,fileName.Data());
    vecMD.push_back(recoFileName);
  }
}

void EXOSourceRunsPolishedInfo::SetDataTreeFileNames(const char* nameOption, const char* dirFiles)
{
  // set tree names with brief explanation of run config
  // file name = dirFiles + nameOption_data_week_87_source_th228_pos_s5_pre_0_run_5030_tree.root
  // the file names are set as part of the source run info with key = "TreeFileName"
  
  std::string key = "DataTreeFileName";

  std::string startName = dirFiles;
  startName.append("/");
  startName.append(nameOption);
  
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;

    std::string fileName = startName;
    
    if(FindMetadata("RunNumber",vecMD))
      fileName += Form("_run_%s",FindMetadata("RunNumber",vecMD)->AsString().c_str());

    
    //if(FindMetadata("CampaignIndex",vecMD))
    //  fileName += Form("_cpg_%s",FindMetadata("CampaignIndex",vecMD)->AsString().c_str());
    
    //if(FindMetadata("WeekIndex",vecMD))
    //  fileName += Form("_week_%s",FindMetadata("WeekIndex",vecMD)->AsString().c_str());

    
    if(FindMetadata("SourceName",vecMD))
    {
      TString sourceStrength= "";
      if(FindMetadata("SourceStrength",vecMD))
        sourceStrength = FindMetadata("SourceStrength",vecMD)->AsString();

      TString sourceName = FindMetadata("SourceName",vecMD)->AsString();
      sourceName = sourceName.ReplaceAll("-","");
      sourceName.ToLower();

      if(sourceStrength != "")
        sourceName = sourceStrength + "_" + sourceName;
      
      fileName += Form("_%s",sourceName.Data());
    }
    
    if(FindMetadata("SourcePositionS",vecMD) && FindMetadata("SourcePositionX",vecMD) && FindMetadata("SourcePositionY",vecMD) && FindMetadata("SourcePositionZ",vecMD))
    {
      TString posS = FindMetadata("SourcePositionS",vecMD)->AsString();
      TString posX = FindMetadata("SourcePositionX",vecMD)->AsString();
      TString posY = FindMetadata("SourcePositionY",vecMD)->AsString();
      TString posZ = FindMetadata("SourcePositionZ",vecMD)->AsString();

      posX.ReplaceAll(".","");
      posY.ReplaceAll(".","");
      posZ.ReplaceAll(".","");

      int x = atoi(posX.Data());
      int y = atoi(posY.Data());
      int z = atoi(posZ.Data());

      posX = "";
      posY = "";
      posZ = "";
      
      if(x)
        posX = (x > 0) ? Form("px%d",abs(x)) : Form("nx%d",abs(x));
      if(y)
        posY = (y > 0) ? Form("py%d",abs(y)) : Form("ny%d",abs(y));
      if(z)
        posZ = (z > 0) ? Form("pz%d",abs(z)) : Form("nz%d",abs(z));

      
      fileName += Form("_s%s",posS.Data());
      //if(posX != "")
      //  fileName += Form("_%s",posX.Data());
      //if(posY != "")
      //  fileName += Form("_%s",posY.Data());
      //if(posZ != "")
      //  fileName += Form("_%s",posZ.Data());
    }
      
    if(FindMetadata("TriggerPrescale",vecMD))
    {
      int trigger = FindMetadata("TriggerPrescale",vecMD)->AsInt();
      TString trigName = (trigger < 0) ? "hz" : "pre"; 
      
      fileName += Form("_%s_%d",trigName.Data(),abs(trigger));
    }

    fileName += "_tree.root";
    
    EXOMetadata treeFileName(key,fileName);
    vecMD.push_back(treeFileName);
  }

  return;  
}

void EXOSourceRunsPolishedInfo::SetDataSelectionFileNames(const char* dataMCname, const char* cutDatadir, const char* multWildcard)
{
  // set name of file name with data selection for MC-based fits
  
  std::string keyCut = "DataSelectionFileName";
  std::string keyWildCard = "DataMultiplicityWildCard";
  std::string keyId = "DataId";
  
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;
    
    if(FindMetadata("DataTreeFileName",vecMD))
    {
      std::string fullDataFileName(FindMetadata("DataTreeFileName",vecMD)->AsString());
      size_t posSlash = fullDataFileName.rfind("/");
      TString dataFileName = fullDataFileName.substr(posSlash+1).c_str();

      TString dataId = dataFileName;
      dataId.ReplaceAll(".root",Form("_%s",multWildcard));
      
      dataFileName.ReplaceAll(".root",Form("_%s_%s.root",dataMCname,multWildcard));
      dataFileName = Form("%s/%s",cutDatadir,dataFileName.Data());

      EXOMetadata metaCut(keyCut,dataFileName.Data());
      EXOMetadata wildMeta(keyWildCard,multWildcard);
      EXOMetadata idMeta(keyId,dataId.Data());

      vecMD.push_back(metaCut);
      vecMD.push_back(wildMeta);
      vecMD.push_back(idMeta);
    }
    else
    {
      std::cerr << "Missing source run info: DataTreeFileName, use SetDataTreeFileNames(nameOption,dirFiles) before calling this function!" << std::endl;      
    }
  }
  return;
}

void EXOSourceRunsPolishedInfo::SetMCTreeFileNames(const char* mcDir)
{
  // set MC file names
  
  std::string key = "MCTreeFileName";
  
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;

    if(FindMetadata("SourceName",vecMD) && FindMetadata("SourcePositionS",vecMD) && FindMetadata("SourcePositionX",vecMD) && FindMetadata("SourcePositionY",vecMD) && FindMetadata("SourcePositionZ",vecMD))
    {
      std::string sourceName = FindMetadata("SourceName",vecMD)->AsString();
      int positionS = FindMetadata("SourcePositionS",vecMD)->AsInt();
      double positionX = FindMetadata("SourcePositionX",vecMD)->AsDouble();
      double positionY = FindMetadata("SourcePositionY",vecMD)->AsDouble();
      double positionZ = FindMetadata("SourcePositionZ",vecMD)->AsDouble();

      std::string mcTreeFileName = GetMCTreeFileName(sourceName.c_str(),positionS,positionX,positionY,positionZ);
      mcTreeFileName = Form("%s/%s",mcDir,mcTreeFileName.c_str());
      
      EXOMetadata mcFileName(key,mcTreeFileName);
      vecMD.push_back(mcFileName);
    }
    else
    {
      std::cerr << "Missing source run info: SourceName or SourcePositionS or SourcePositionX or SourcePositionY or SourcePositionZ" << std::endl;
    }
  }
  return;
}

void EXOSourceRunsPolishedInfo::SetMCSelectionFileNames(const char* cutMCname, const char* cutMCdir, const char* multWildcard)
{
  // set name of file name with MC selection for MC-based fits
  // also set MC id
  
  std::string keyCut = "MCSelectionFileName";
  std::string keyId = "MCId";
  std::string keyWildCard = "MCMultiplicityWildCard";

  TString strCutMCname = cutMCname ? cutMCname : "";  
  
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;
    
    if(FindMetadata("MCTreeFileName",vecMD))
    {
      std::string fullMCFileName(FindMetadata("MCTreeFileName",vecMD)->AsString());
      size_t posSlash = fullMCFileName.rfind("/");
      TString mcFileName = fullMCFileName.substr(posSlash+1).c_str();
      
      TString mcId = mcFileName;
      mcId.ReplaceAll(".root",Form("_%s",multWildcard));
      
      //mcFileName.ReplaceAll(".root",Form("_%s_%s.root",cutMCname,multWildcard));
      mcFileName = Form("%s/%s",cutMCdir,mcFileName.Data());
      if(strCutMCname != "")
	mcFileName.ReplaceAll(".cut.root",strCutMCname.Data()); // manuel updated to include .cut.root as the file ending... in r11185 ... and having .cut.cut breaks things. -- caio reverted this and changed the filename ending in r11226 to just .cut.root when non empty -- then manuel reupdated in r11238 to append a default .cut.root for the selection file name -- which works best for him as he runs with the default strcutmcname of ''


      EXOMetadata metaId(keyId,mcId.Data());
      EXOMetadata metaCut(keyCut,mcFileName.Data());
      EXOMetadata wildMeta(keyWildCard,multWildcard);

      vecMD.push_back(metaId);
      vecMD.push_back(metaCut);
      vecMD.push_back(wildMeta);
    }
    else
    {
      std::cerr << "Missing source run info: MCTreeFileName, use SetMCTreeFileNames(mcDir) before calling this function!" << std::endl;      
    }
  }
  return;
}

const std::string EXOSourceRunsPolishedInfo::GetMCTreeFileName(TString sourceName, int positionS, double positionX, double positionY, double positionZ)
{
  positionX = 0.0;
  positionY = 0.0;

  sourceName.ReplaceAll("-","");
  
  TString fileName = Form("SourceS%d_%s",positionS,sourceName.Data());
  
  if(positionS == 5)
  {
    if(fabs(positionZ) < 1. && sourceName != "Ra226") // new S5, except for Ra226 MC which is inverted
    {
      fileName += "_px2550_py39";
    }
    if(fabs(positionZ) > 1. && sourceName == "Ra226")
    {
      fileName += "_px2550_py39_nz300";
    }
  }
  else if(positionS == 8)
  {
    if(sourceName == "Ra226")
    {
      fileName += "_px250_py23_pz2953";
    }
  }

  fileName += ".cut.root"; // this line cannot be changed independently of l582 
  
  return fileName.Data();
}

void EXOSourceRunsPolishedInfo::SetNoiseLevelPeriods(bool apdOrWire, bool splitMed, bool splitPos)
{
  if(apdOrWire)
    SetAPDNoiseLevelPeriods(splitMed,splitPos);
  else
    SetWiresNoiseLevelPeriods();
}

void EXOSourceRunsPolishedInfo::SetWiresNoiseLevelPeriods()
{
  // combine the noise indices in Phase1 (indices 1-6) into levels (low or high)
  // high noise: 2,4 and 5
  // low noise: 1,3 and 6

  std::vector<std::string> noiseIndexList = GetListOf("NoiseIndex");
  std::vector<std::string> noiseLivetimeList = GetListOf("NoiseLivetime");
  
  std::map<int, double> noiseLivetime;
  for(size_t i = 0; i < noiseIndexList.size(); i++)
  {
    int index = atoi(noiseIndexList[i].c_str());
    if(noiseLivetime.count(index) <= 0)
      noiseLivetime[index] = atof(noiseLivetimeList[i].c_str());
  }

  double noiseHigh = noiseLivetime[2]+noiseLivetime[4]+noiseLivetime[5];
  double noiseLow = noiseLivetime[1]+noiseLivetime[3]+noiseLivetime[6];

  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;

    const EXOMetadata* md = FindMetadata("NoiseIndex",vecMD);
    if(md)
    {
      if(md->AsInt() == 2 || md->AsInt() == 4 || md->AsInt() == 5)
      {
        EXOMetadata nlMD("NoiseLevel","High");
        vecMD.push_back(nlMD);
        EXOMetadata ltMD("NoiseLevelLivetime",Form("%.2f",noiseHigh));
        vecMD.push_back(ltMD);
      }
      else if(md->AsInt() == 1 || md->AsInt() == 3 || md->AsInt() == 6)
      {
        EXOMetadata nlMD("NoiseLevel","Low");
        vecMD.push_back(nlMD);
        EXOMetadata ltMD("NoiseLevelLivetime",Form("%.2f",noiseLow));
        vecMD.push_back(ltMD);
      }
    }
  }      
}

void EXOSourceRunsPolishedInfo::SetAPDNoiseLevelPeriods(bool splitMed, bool splitPos)
{
  // combine the noise indices in Phase1 (indices 1-6) into levels (low or high)
  // low noise are indices: 1,4 and (if splitMed) 20% of 2 <-- percentage more or less checked with MC
  // high noise is: 3,5,6 and (if splitMed) 80% of 2
  // med noise is: 2

  std::vector<std::string> noiseIndexList = GetListOf("NoiseIndex");
  std::vector<std::string> noiseLivetimeList = GetListOf("NoiseLivetime");
  
  std::map<int, double> noiseLivetime;
  for(size_t i = 0; i < noiseIndexList.size(); i++)
  {
    int index = atoi(noiseIndexList[i].c_str());
    if(noiseLivetime.count(index) <= 0)
      noiseLivetime[index] = atof(noiseLivetimeList[i].c_str());
  }

  double noiseLow = noiseLivetime[1]+noiseLivetime[4];//+0.2*noiseLivetime[2];
  double noiseHigh = noiseLivetime[3]+noiseLivetime[5]+noiseLivetime[6];//+0.8*noiseLivetime[2];
  double noiseMed = noiseLivetime[2];
  if(splitMed)
  {
    noiseLow += 0.2*noiseLivetime[2];
    noiseHigh += 0.8*noiseLivetime[2];
  }

  TRandom3 rdm;
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;

    const EXOMetadata* mdni = FindMetadata("NoiseIndex",vecMD);
    const EXOMetadata* mdpos = FindMetadata("SourcePositionS",vecMD);
    if(mdni && (mdpos || !splitPos))
    {
      std::string pos = "";
      if(splitPos)
        pos = (mdpos->AsInt() == 2 || mdpos->AsInt() == 8) ? "_Anode" : "_Cathode";
            
      if(mdni->AsInt() == 1 || mdni->AsInt() == 4)
      {
        EXOMetadata nlMD("NoiseLevel",Form("Low%s",pos.c_str()));
        vecMD.push_back(nlMD);
        EXOMetadata ltMD("NoiseLevelLivetime",Form("%.2f",noiseLow));
        vecMD.push_back(ltMD);        
      }
      else if(mdni->AsInt() == 3 || mdni->AsInt() == 5 || mdni->AsInt() == 6)
      {
        EXOMetadata nlMD("NoiseLevel",Form("High%s",pos.c_str()));
        vecMD.push_back(nlMD);
        EXOMetadata ltMD("NoiseLevelLivetime",Form("%.2f",noiseHigh));
        vecMD.push_back(ltMD);
      }
      else // noise == 2
      {
        if(splitMed)
        {
          double p = rdm.Uniform();
          EXOMetadata nlMD("NoiseLevel",Form("%s",(p<0.2)?Form("Low%s",pos.c_str()):Form("High%s",pos.c_str())));
          vecMD.push_back(nlMD);
          EXOMetadata ltMD("NoiseLevelLivetime",Form("%.2f",(p<0.2)?noiseLow:noiseHigh));
          vecMD.push_back(ltMD);
        }
        else
        {
          EXOMetadata nlMD("NoiseLevel",Form("Medium%s",pos.c_str()));
          vecMD.push_back(nlMD);
          EXOMetadata ltMD("NoiseLevelLivetime",Form("%.2f",noiseMed));
          vecMD.push_back(ltMD);          
        }
      }
    }
  }      
}

void EXOSourceRunsPolishedInfo::SetSpatialDistribution()
{
  // combine S5+S11 runs into Cathode
  // and S2+S8 runs into Anode

  std::vector<std::string> weekIndexList = GetListOf("WeekIndex");
  std::vector<std::string> weekLivetimeList = GetListOf("WeekLivetime");

  double totLivetime = 0.;
  std::set<int> usedWeek;
  for(size_t i = 0; i < weekIndexList.size(); i++)
  {
    int index = atoi(weekIndexList[i].c_str());
    if(usedWeek.count(index) <= 0)
    {
      usedWeek.insert(index);
      totLivetime += atof(weekLivetimeList[i].c_str());
    }
  }

  
  for(std::map<Int_t, std::vector<EXOMetadata> >::iterator runMD = fAllMD.begin(); runMD != fAllMD.end(); runMD++)
  {
    std::vector<EXOMetadata> & vecMD = runMD->second;

    const EXOMetadata* mdpos = FindMetadata("SourcePositionS",vecMD);
    const EXOMetadata* mdsrc = FindMetadata("SourceName",vecMD);
    if(mdpos && mdsrc)
    {
      std::string pos = "";
      if(mdpos->AsInt() == 2 || mdpos->AsInt() == 8)
        pos = "Anode";
      else if(mdpos->AsInt() == 5 || mdpos->AsInt() == 11)
        pos = "Cathode";
      else
        pos = "Other";
      
      EXOMetadata sdMD("SpatialDistribution",Form("%s_%s",pos.c_str(),mdsrc->AsString().c_str()));
      vecMD.push_back(sdMD);
      EXOMetadata ltMD("SpatialDistributionLivetime",Form("%.2f",totLivetime));
      vecMD.push_back(ltMD);        
    }
  }
}
