#include "EXOSourceAgreement1D.hh"

ClassImp(EXOSourceAgreement1D)

EXOSourceAgreement1D::EXOSourceAgreement1D(const char* sourceRunsInfoFileName)
: EXOSourceAgreement(sourceRunsInfoFileName)
{
  Init();
}

EXOSourceAgreement1D::EXOSourceAgreement1D(EXOSourceRunsPolishedInfo& sourceRunsInfo)
  : EXOSourceAgreement(sourceRunsInfo)
{
  Init();
}

EXOSourceAgreement1D::EXOSourceAgreement1D(const char* filename, const char* wspname, const char* pdfname, bool readObs)
  : EXOSourceAgreement(filename,wspname,pdfname,readObs)
{
  Init();
}

void EXOSourceAgreement1D::Init()
{
  fDimension = 1;
  SetExtraCalib(1.0, 0.0, 1.0, 0.0);
}

EXOSourceAgreement1D::~EXOSourceAgreement1D()
{
}

TH1D* EXOSourceAgreement1D::CreateHisto(RooAbsBinning& binning, TString name, TString suffix)
{
  // make empty histogram with appropriate binnning to handle variably sized bins
  suffix.ToLower();
  TString id = Form("%s_%s",name.Data(),suffix.Data());

  //std::cout << "Creating TH1D : " << id << std::endl;
  TH1D* histo = new TH1D(id.Data(),"",binning.numBins(),binning.array());

  return histo;
}

const char* EXOSourceAgreement1D::GetName(RooRealVar& var, TString sep)
{
  // append lower case separation...
  sep.ToLower();
  return Form("%s_%s",var.GetName(),sep.Data());
}

bool EXOSourceAgreement1D::FillHistos()
{
  std::cout << "Filling data and MC 1D histograms ..." << std::endl;
  fObsList.Print();
  // old way of iterating --> now use paired list for filtering etc.
  // for(int i = 0; i < fObsList.getSize(); i++){
  //   for(std::set<TString>::iterator sep = fSeparation.begin(); sep != fSeparation.end(); sep++){
  //   }
  // }
  for (int j = 0 ; j < (int) fObsSepPairs.size() ; j++ ) { // only for variables limited to
      int i = (fObsSepPairs.at(j)).first;
      TString* sep = &(fObsSepPairs.at(j).second);
      std::cout << i << ", " << sep->Data() << std::endl;
      RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(i));
      if (var) { 
	if ( FillHistos(*var,sep->Data() ) ){
	  TString name( GetName(*var,sep->Data() ) );
	  //std::cout << "Filled Histo : " << name.Data() << std::endl;
	  TH1D* dataHisto = dynamic_cast<TH1D*>(fDataHisto[name]);
	  TH1D* mcHisto   = dynamic_cast<TH1D*>(fMCHisto[  name]);
	  double* activity = &fWeightedActivity[name];
	  
	  if(not (dataHisto and mcHisto and activity))
	    return false;
	  
	  //note default is 1 ie no rebinning in place
	  dataHisto->Rebin(fRebinX);
	  mcHisto->Rebin(  fRebinX);
	  
	  //double norm = dataHisto->Integral();
	  //if(norm > 0)
	  //{
	  // set histograms to fractional, but entries to data. 
	  if(dataHisto->GetEntries() > 0 or dataHisto->Integral() <= 0){
	    double totalData = dataHisto->Integral();
	    dataHisto->Scale(  1./totalData);
	    dataHisto->SetEntries(totalData);
	    
	    double totalMC = mcHisto->Integral();
	    mcHisto->Scale(  1./totalMC);
	    mcHisto->SetEntries(totalMC);

	    //std::cout << "total MC " << totalMC << std::endl;
	    
	    //std::cout << "norm factor data " << dataHisto->GetNormFactor() << " mc " << dataHisto->GetNormFactor() << std::endl;
	  } else{
	    std::cout <<"Histo unfilled " << std::endl;
	    dataHisto->SetEntries(0);
	    mcHisto->SetEntries(  0);
	  }
	  //  (*activity) /= norm;
	  //std::cout << "scaling " << var->GetName() << " , " << sep->Data() << " by " << 1/norm << std::endl;
	  
	  //	  if (fUserEnergyCut.IsNull()){
	  //CleanUpMCbins(dataHisto,mcHisto);
	  for(int b = 1; b <= mcHisto->GetNbinsX(); b++){
	    double dataError = dataHisto->GetBinError(b);
	    if(dataError <= 0){
	      std::cout << "zeroing from no error : "<< b << std::endl;
	      mcHisto->SetBinContent(b,0);
	      mcHisto->SetBinError(b,0);
	    }
	  }
	} else  {
	  return false;
	}
      }
  }
  return true;
}

bool EXOSourceAgreement1D::EvalShape(){
  std::cout << "Evaluating shape agreement 1D ..." << std::endl;
  
  for (int j = 0 ; j < (int) fObsSepPairs.size() ; j++ ) {
      int i = (fObsSepPairs.at(j)).first;
      TString* sep = &(fObsSepPairs.at(j).second);
      RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(i));
      if (var){
	if (not EvalShapeFor(*var,sep->Data() ) )
	  return false;
      }
  }
  return true;
}

bool EXOSourceAgreement1D::EvalShapeFor(RooRealVar& var, TString sep)
{
  std::cout << Form("Evaluating shape agreement 1D for observable %s and separation %s ...",
		    var.GetName(),sep.Data()) << std::endl;
  sep.ToLower();

  TString name(GetName(var,sep));

  TH1D* dataHisto = dynamic_cast<TH1D*>(fDataHisto[name]);
  TH1D* mcHisto   = dynamic_cast<TH1D*>(fMCHisto[  name]);

  TH1D* dataHistoShape = dynamic_cast<TH1D*>(dataHisto->Clone(Form("%s_shape_data",name.Data())));
  TH1D* mcHistoShape   = dynamic_cast<TH1D*>(  mcHisto->Clone(Form("%s_shape_mc",name.Data())));

  if(not MakeComparisonHistos(dataHistoShape,mcHistoShape,"shape",name.Data()))
    return false;
  
  return true;
}

bool EXOSourceAgreement1D::EvalSSFractions()
{
  // to perform SS fraction agreement, we need both energy_ss and energy_ms
  bool hasSS = (fSeparation.count("SS") > 0);
  bool hasMS = (fSeparation.count("MS") > 0);

  if(hasSS and hasMS)
  {
    std::cout << "Evaluating SS-fraction agreement 1D ..." << std::endl;
  }
  else
  {
    std::cout << "SS-fraction can only be evaluated if separation contains both SS and MS!" << std::endl;
    return false;
  }
  
  bool result = true;

  for (int j = 0 ; j < (int) fObsSepPairs.size() ; j++ ) { // need to check the limited list not the fObsList.
    int i = (fObsSepPairs.at(j)).first;
    TString* sep = &(fObsSepPairs.at(j).second);
    if (sep->CompareTo("SS") ) { // only do it for SS -> handles pairing
      continue; }
    RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(i));
    if (var) {
      TString varName = var->GetName();
      if (varName.EndsWith("_SS", TString::kIgnoreCase ) ){ //TString::ECaseCompare::kIgnoreCase
	varName.Resize(varName.Length()-3);
	result = EvalSSFractionsFor(Form("%s_ss_ss",varName.Data()),
				    Form("%s_ms_ms",varName.Data()),
				    Form("%s_all"  ,varName.Data())) && result;
      }else {
	result = EvalSSFractionsFor(GetName(*var,"SS"),
				    GetName(*var,"MS"),
				    var->GetName()   ) && result;
      }
    }
  }
  return result;
}

bool EXOSourceAgreement1D::EvalSSFractionsFor(TString nameSS, TString nameMS, TString name)
{
  std::cout << Form("Evaluating SS-fraction agreement for %s , %s and %s ... ",
		    nameSS.Data(), nameMS.Data(), name.Data()) << std::endl;

  TH1D* dataHistoSS = dynamic_cast<TH1D*>(fDataHisto[nameSS]);
  TH1D* mcHistoSS = dynamic_cast<TH1D*>(fMCHisto[nameSS]);
  TH1D* dataHistoMS = dynamic_cast<TH1D*>(fDataHisto[nameMS]);
  TH1D* mcHistoMS = dynamic_cast<TH1D*>(fMCHisto[nameMS]);

  if (not dataHistoSS or not mcHistoSS or not dataHistoMS or not mcHistoMS ) {
      std::cout << dataHistoSS << mcHistoSS << dataHistoMS << mcHistoMS <<std::endl;
      return false;
  }

  TH1D* dataHistoFrac = dynamic_cast<TH1D*>(dataHistoSS->Clone(Form("%s_ssfrac_data",name.Data())));
  TH1D* mcHistoFrac = dynamic_cast<TH1D*>(mcHistoSS->Clone(Form("%s_ssfrac_mc",name.Data())));

  TH1D* dataHistoTot = dynamic_cast<TH1D*>(dataHistoMS->Clone(Form("%s_total_data",name.Data())));
  TH1D* mcHistoTot = dynamic_cast<TH1D*>(mcHistoMS->Clone(Form("%s_total_mc",name.Data())));

  dataHistoFrac->Scale(dataHistoSS->GetEntries());
  dataHistoTot->Scale(dataHistoMS->GetEntries());
  mcHistoFrac->Scale(mcHistoSS->GetEntries());
  mcHistoTot->Scale(mcHistoMS->GetEntries());

  dataHistoTot->Add(dataHistoFrac);
  dataHistoFrac->Divide(dataHistoTot);
  //AddResultHisto(dataHistoFrac);

  mcHistoTot->Add(mcHistoFrac);
  mcHistoFrac->Divide(mcHistoTot);
  //AddResultHisto(mcHistoFrac);

  //fAgreements.at("ssfrac").push_back(name.Data());

  return MakeComparisonHistos(dataHistoFrac,mcHistoFrac,"ssfrac",name.Data());
}

bool EXOSourceAgreement1D::FillHistos(RooRealVar& var, TString sep)
{
  // two cases source and low background

  std::cout <<
    Form("Filling data and MC 1D histograms for observable %s and separation %s ...",
	 var.GetName(),sep.Data())
	    << std::endl;

  if(fAgreeType == kSource)
    return FillSourceHistos(var,sep);

  if(fAgreeType == kLB)
    return FillLBHistos(var,sep);

  return false;
}

bool EXOSourceAgreement1D::FillSourceHistos(RooRealVar& var, TString sep)
{
  std::cout << Form("Filling source data and MC 1D histograms for observable %s and separation %s ...",
		    var.GetName(),sep.Data()) << std::endl;
  
  sep.ToLower();
  RooAbsBinning& binning = var.getBinning();
  int multiplicity = (sep == "ss") ? 1 : 2;
  TString name(GetName(var,sep));

  if(fDataHisto.count(name))
    delete fDataHisto[name];
  fDataHisto[name] = CreateHisto(binning,name,"data");
  TH1D* dataHisto = dynamic_cast<TH1D*>(fDataHisto[name]);

  if(fMCHisto.count(name))
    delete fMCHisto[name];
  fMCHisto[name] = CreateHisto(binning,name,"mc");
  TH1D* mcHisto = dynamic_cast<TH1D*>(fMCHisto[name]);

  fWeightedActivity[name] = 0.;
  double* activity = &fWeightedActivity[name];

  std::vector<std::string> mcFiles = fRunsInfo.GetListOf("MCSelectionFileName");
  std::vector<std::string> dataFiles = fRunsInfo.GetListOf("DataSelectionFileName");
  std::vector<std::string> runNumbers = fRunsInfo.GetListOf("RunNumber");
  std::vector<std::string> startTimes = fRunsInfo.GetListOf("StartTimeSec");
  std::vector<std::string> weeks = fRunsInfo.GetListOf("WeekIndex");
  std::vector<std::string> activities = fRunsInfo.GetListOf("IntegratedActivity");
  std::vector<std::string> exposures = fRunsInfo.GetListOf("Exposure");

  std::vector<std::string> groups = fRunsInfo.GetListOf(fGroupName.Data());
  std::vector<std::string> weights = fRunsInfo.GetListOf(fGroupWeight.Data());

  std::map<TString, TH1D*> groupData;
  std::map<TString, TH1D*> groupMC;
  std::map<TString, double> groupMCentries;
  std::map<TString, double> groupDataExposure;
  std::map<TString, double> groupActivity;

  std::set<TString> groupNames(groups.begin(),groups.end());
  for(std::set<TString>::iterator groupName = groupNames.begin(); groupName != groupNames.end(); groupName++)
  {
    groupData.insert(std::make_pair(groupName->Data(),CreateHisto(binning,groupName->Data(),"data")));
    groupMC.insert(std::make_pair(groupName->Data(),CreateHisto(binning,groupName->Data(),"mc")));
    groupMCentries.insert(std::make_pair(groupName->Data(),0.));
    groupDataExposure.insert(std::make_pair(groupName->Data(),0.));
    groupActivity.insert(std::make_pair(groupName->Data(),0.));
  }
  
  TString cutData = GetObservableLimitsCut(GetObservablesIn(fObsList,sep,"data"));
  //std::cout << "cutData: " << cutData.Data() << std::endl;
  TString cutMC =   GetObservableLimitsCut(GetObservablesIn(fObsList,sep,"mc"  ));
  //std::cout << "cutMC: "   << cutMC.Data()   << std::endl;


  //std::cout << " Before adding cut " << cutMC.Data() << std::endl;

  //extra fiducial cut for testing
  //cutData += " && Max$(cluster_x*cluster_x*(abs(cluster_x)<900) + cluster_y*cluster_y*(abs(cluster_y)<900)) < 26244";
  //cutMC   += "Max$(cluster_x*cluster_x*(abs(cluster_x)<900) + cluster_y*cluster_y*(abs(cluster_y)<900)) < 26244";
 
  //cutData += " && Max$(abs(cluster_z)) < 172";
  //cutMC   += "Max$(abs(cluster_z)) < 172";

  //cutData += " && Max$(cluster_x*cluster_x*(abs(cluster_x)<900) + cluster_y*cluster_y*(abs(cluster_y)<900)) < 26244 && Max$(abs(cluster_z)) < 172";
  //cutMC   += "Max$(cluster_x*cluster_x*(abs(cluster_x)<900) + cluster_y*cluster_y*(abs(cluster_y)<900)) < 26244 && Max$(abs(cluster_z)) < 172";

  //std::cout << " After adding cut " << cutMC.Data() << std::endl;

  //Max$(abs(cluster_z)) < 172


  std::map<TString, TH1D> processedMCs;
  std::map<TString, double> entriesMCs;
  std::map<TString, double> effMCs;
  size_t nRuns = runNumbers.size();
  if(nRuns == 0){
    std::cout << "no runs" << std::endl;
    return true;
  }
  for(size_t iRun = 0; iRun < nRuns; iRun++)
  {
    std::cout << "Num W: " << weights.size() << " G: " << groups.size() << std::endl; 
    std::cout << Form("Working on run : %s , group : %s , weight : %s , cut : %s ",
		      runNumbers[iRun].c_str(),
		      groups[iRun].c_str(),
		      weights[iRun].c_str(),
		      cutData.Data()) << std::endl;
    // data histo
    std::cout << "Looking at file: " << dataFiles[iRun].c_str() << std::endl;
    TFile dataFile(dataFiles[iRun].c_str(),"read");
    if(dataFile.IsZombie())
      continue;
    TEntryList* dataSelection = ((sep=="all") ? 
				 dynamic_cast<TEntryList*>(dataFile.Get("EventList")) : 
				 dynamic_cast<TEntryList*>(dataFile.Get(Form("EventList_%s",sep.Data()))) );
    if(not dataSelection)
      continue;
    std::cout << "With tree name: " << dataSelection->GetTreeName() 
	      << " and preprocced file: " << dataSelection->GetFileName() << std::endl;
    TChain dataChain(dataSelection->GetTreeName());
    int fileOk = dataChain.Add(dataSelection->GetFileName(),0);
    if(not fileOk)
      continue;
    dataChain.SetEntryList(dataSelection);
    dataChain.SetEstimate(dataSelection->GetN()+1);
    
    double a = 1.0;
    double b = 0.0;
    TString varTest = var.GetName();
    if (varTest.Contains("ss"))
    {
        a=aSS;
        b=bSS;
    }
    else if (varTest.Contains("ms"))
    {
        a = aMS;
        b = bMS;
    }
    std::cout << "Draw command is "<< Form("(%s*%f + %f):1",var.GetName(),a,b) << std::endl;

    dataChain.Draw(Form("(%s*%f + %f):1",var.GetName(),a,b),cutData.Data(),"goff");
    
    TH1D* hData = CreateHisto(var.getBinning(),runNumbers[iRun],"data");
    hData->FillN(dataChain.GetSelectedRows(),dataChain.GetV1(),dataChain.GetV2());
    hData->Sumw2();
    
    // mc histo
    TString mcName = 
      fUseWeeklyResol ? Form("%s_%s",mcFiles[iRun].c_str(),weeks[iRun].c_str()) : mcFiles[iRun].c_str();
    if(processedMCs.count(mcName) <= 0)
    {
      std::cout << Form("Working on MC file : %s , for week : %s , w/ cut : %s ...",
			mcFiles[iRun].c_str(),weeks[iRun].c_str(),cutMC.Data())
		<< std::endl;
      TH1D* hMC = CreateHisto(var.getBinning(),mcName.Data(),"mc");
      
      TFile *mcFile = TFile::Open(mcFiles[iRun].c_str(),"read");
      if(mcFile->IsZombie())
        continue;
      
      TEntryList* mcSelection = ((sep=="all") ? 
				 dynamic_cast<TEntryList*>(mcFile->Get("EventList")) : 
				 dynamic_cast<TEntryList*>(mcFile->Get(Form("EventList_%s",sep.Data()))) );
      if(not mcSelection)
        continue;

      TString treeName = mcSelection->GetTreeName();
      TString fileName = mcSelection->GetFileName();
      
      TChain *mcChain = new TChain(treeName.Data());
      int mcOk = mcChain->Add(fileName.Data(),0);
      if(not mcOk)
        continue;

      mcChain->SetEntryList(mcSelection);
      mcChain->SetEstimate(mcSelection->GetN()+1);

      TString varName = var.GetName();
      if(varName.Contains("energy"))
      {
        std::cout << var.GetName() << std::endl;
        std::cout << "Mult = " << multiplicity  << std::endl;
        std::cout << "Seconds = " << atoi(startTimes[iRun].c_str())  << std::endl;
        //There is a maximum for the length of vector declaration since it pre-allocs memory
        //if using a cluster level thing we may hit this limit.
        mcChain->Draw("energy_mc:weight",cutMC.Data(),"goff");
        std::cout << "Length = " << mcChain->GetSelectedRows()  << std::endl;
        fResolution->FillSmearedMCHisto1D(*hMC, "Rotated", mcChain->GetV1(), mcChain->GetV2(), 
					  mcChain->GetSelectedRows(), multiplicity, 
					  atoi(startTimes[iRun].c_str()), 0, 1);
      }
      else
      {
        std::cout << var.GetName() << std::endl;
        
        mcChain->Draw(Form("%s:energy_mc:weight",var.GetName()),cutMC.Data(),"goff");
        std::cout << "Length = " << mcChain->GetSelectedRows()  << std::endl;

        FillSmearedMCHisto1D(*hMC, mcChain->GetV1(), GetEnergyObservableIn(GetObservablesIn(fObsList,sep,"data")),
			     "Rotated", mcChain->GetV2(), mcChain->GetV3(), mcChain->GetSelectedRows(),
			     multiplicity, atoi(startTimes[iRun].c_str()), 0, 1);
        std::cout << "Obtained " << var.GetName()  << std::endl;
        //hMC->FillN(mcChain.GetSelectedRows(),mcChain.GetV1(),mcChain.GetV2());
      }

      mcChain->Delete();

      hMC->Sumw2();
      double totMC = hMC->Integral();
      hMC->Scale(1./totMC);
      
      processedMCs[mcName] = *hMC;
      entriesMCs[mcName] = totMC;

      if(mcFile and !mcFile->IsZombie())
      {
        mcFile->Close();
      }

      TFile *fullMCfile = TFile::Open(fileName.Data());
      TParameter<Long64_t>* nMCevents = dynamic_cast<TParameter<Long64_t>* >(fullMCfile->Get("nMCEvents"));
      effMCs[mcName] = nMCevents ? totMC*1./nMCevents->GetVal() : 0.;

      if(fullMCfile and !fullMCfile->IsZombie())
      {
        fullMCfile->Close();
      }
      
      std::cout << Form("Done working on MC file : %s , for week : %s , w/ cut : %s ...",
			mcFiles[iRun].c_str(),weeks[iRun].c_str(),cutMC.Data()) << std::endl;
    }

    groupData[groups[iRun]]->Add(hData);

    //double normMC = hData->Integral()/processedMCs[mcName].Integral();
    groupDataExposure[groups[iRun]] += atof(exposures[iRun].c_str())*1e-9; // convert from nanoseconds into days
    //double counts = hData->Integral();
    groupMC[groups[iRun]]->Add(&processedMCs[mcName],hData->Integral());//counts);//normMC);
    groupMCentries[groups[iRun]] += entriesMCs[mcName]*hData->Integral();//counts;
    //groupMC[groups[iRun]]->SetNormFactor(groupMC[groups[iRun]]->GetNormFactor()+processedMCs[mcName].GetNormFactor());
    //std::cout << "current group " << groups[iRun] << " norm factor = " << groupMC[groups[iRun]]->GetNormFactor() << std::endl;
    groupActivity[groups[iRun]] += atof(activities[iRun].c_str())*effMCs[mcName];
    
    delete hData;
  }

  // get weights and create storage functions
  std::map<std::string, double> groupWeights;
  for(size_t iRun = 0; iRun < nRuns; iRun++)
    groupWeights[groups[iRun]] = atof(weights[iRun].c_str());
  
  TString expression = "[0] + 0.";
  double sumWeights = 0.;
  for(std::map<std::string, double>::iterator group = groupWeights.begin(); group != groupWeights.end(); group++)
  {
    int iGroup = std::distance(groupWeights.begin(),group);
    expression += Form("*[%u]",static_cast<unsigned int>(iGroup+1));
    double groupWeight = group->second;
    sumWeights += groupWeight;
    //std::cout << iGroup << " in group " << group->first << " weight " << groupWeight << std::endl;
  }
  
  TF1* dataEntriesFunc = new TF1("dataEntries",expression.Data(),0,10000);
  TF1* dataWeightsFunc = new TF1("dataWeights",expression.Data(),0,10000);
  TF1* dataExposuresFunc = new TF1("dataExposures",expression.Data(),0,10000);
  TF1* mcEntriesFunc = new TF1("mcEntries",expression.Data(),0,10000);
  TF1* mcWeightsFunc = new TF1("mcWeights",expression.Data(),0,10000);
  
  //Sum The Histograms from each group using correct Weights
  //double totalCounts = 0.;
  double dataActivity = 0.;
  double mcActivity = 0.;
  for(std::map<std::string, double>::iterator group = groupWeights.begin(); group != groupWeights.end(); group++)
  {
    int iGroup = std::distance(groupWeights.begin(),group);
    std::string groupName = group->first;
    double groupWeight = group->second;
    
    double counts = groupData[groupName]->Integral();
    double exposure = groupDataExposure[groupName];
    double sims = groupMCentries[groupName];//groupMC[groupName]->Integral();
    double weight = groupWeight;
    dataEntriesFunc->FixParameter(iGroup+1,counts);
    dataWeightsFunc->FixParameter(iGroup+1,weight);
    dataExposuresFunc->FixParameter(iGroup+1,exposure);
    mcEntriesFunc->FixParameter(iGroup+1,sims);
    mcWeightsFunc->FixParameter(iGroup+1,weight);
    
    if(counts <= 0){
      std::cout << "Non-positive counts : " << groupName << std::endl;
      continue;
    }

    //This scales the Data so the exposure is the correct # of LT days
    double dataRate = counts/exposure;
    //std::cout << "data Rate " << dataRate << std::endl;
    double weightedCounts = dataRate * weight;
    weightedCounts *= 24*60*60; // convert weight in livetime days to exposure in seconds
    //std::cout << "wt Cts " << weightedCounts << std::endl;
    dataHisto->Add(groupData[groupName],weightedCounts/counts);

    //MC already normed to area 1.0
    double mcRate = sims/counts;
    double weightedSims = mcRate*weight/sumWeights;
    mcHisto->Add(groupMC[groupName],weightedSims/counts);//weightedCounts/counts);
    
    dataActivity += counts;
    //    std::cout << "activity so far  : " << dataActivity <<std::endl;
    mcActivity += groupActivity[groupName];
  }

  // add info into TH1 with by fitting the function with one free par [0]
  dataHisto->Fit(dataEntriesFunc,"Q0+");
  dataHisto->Fit(dataWeightsFunc,"Q0+");
  dataHisto->Fit(dataExposuresFunc,"Q0+");
  mcHisto->Fit(mcEntriesFunc,"Q0+");
  mcHisto->Fit(mcWeightsFunc,"Q0+");
  
  //std::cout << name << " acts " << dataActivity << " " << mcActivity <<  " " << dataActivity*1./mcActivity << std::endl;
  (*activity) = mcActivity > 0 ? dataActivity*1./mcActivity : -1.;
  
  DeleteHistos1D(groupData);
  DeleteHistos1D(groupMC);
  processedMCs.clear();

  return true;
}

bool EXOSourceAgreement1D::FillLBHistos(RooRealVar& var, TString sep)
{
  // create histograms from the workspace provided -- for bb2n agreement

  std::cout << Form("Filling LB data and MC 1D histograms for observable %s and separation %s ...",
		    var.GetName(),sep.Data() ) << std::endl;

  if(fWspFileName == "" or fWspName == "")
    return true;
  
  sep.ToLower();
  RooAbsBinning& binning = var.getBinning();
  //int multiplicity = (sep == "ss") ? 1 : 2;
  TString name(GetName(var,sep));

  // prepare histograms in array, empty with binning from var
  if(fDataHisto.count(name))
    delete fDataHisto[name];
  fDataHisto[name] = CreateHisto(binning,name,"data");
  TH1D* dataHisto = dynamic_cast<TH1D*>(fDataHisto[name]);

  if(fMCHisto.count(name))
    delete fMCHisto[name];
  fMCHisto[name] = CreateHisto(binning,name,"mc");
  TH1D* mcHisto = dynamic_cast<TH1D*>(fMCHisto[name]);

  fWeightedActivity[name] = 0.;
  //double* activity = &fWeightedActivity[name];

  TFile wspFile(fWspFileName.Data(),"read");
  RooWorkspace* wsp = dynamic_cast<RooWorkspace*>(wspFile.Get(fWspName.Data()));
  std::cout << "Using LB Workspace " << fWspFileName.Data() << std::endl;

  //EXOWorkingFile pdfFile(EXOWorkingFile::kWorkspace, fFOVs.GetStringOption("fPdfsFileName").c_str());
  //EXOPdfInfoSet* pdfInfoSet = pdfFile.GetPdfInfoSet();
  //EXOPdfInfo* pdfInfo = pdfInfoSet.find("bb2n") 

  //////////////////////////////////////////
  // because of old version of fitting package, some generic objects crash....
  // assume pdfs and datasets are named: pdf_(sep) and data_(sep) respectively
  //TClonesArray* nll_pdfs = dynamic_cast<TClonesArray*>(wspFile.Get("nll_pdfs"));
  //TClonesArray* nll_datasets = dynamic_cast<TClonesArray*>(wspFile.Get("nll_datasets"));
  //TNamed* pdfName = dynamic_cast<TNamed*>(nll_pdfs->FindObject(sep.Data()));
  //RooAbsPdf* pdf = wsp->pdf(pdfName->GetTitle());
  //TNamed* dsName = dynamic_cast<TNamed*>(nll_datasets->FindObject(sep.Data()));
  //RooAbsData* ds = wsp->data(dsName->GetTitle());
  //////////////////////////////////////////

  RooAbsPdf* pdf = wsp->pdf(Form("pdf_%s",sep.Data())); //pdf->Print();
  RooAbsData* data = wsp->data(Form("data_%s",sep.Data())); //data->Print();

  RooArgSet* pdfObs = pdf->getObservables(data);
  RooAbsArg* varObs = pdfObs->find(var.GetName());

  bool makeHistos = varObs ? true : false;
  if(not makeHistos){
    delete pdfObs; // unclear on the need to do this...
    return true;
  }
  RooRealVar* eVar = GetEnergyObservableIn(RooArgList(*pdfObs)); //check energy variable

  RooArgSet projVars ; // set up iters to fill this
  TIterator* piter = pdfObs->createIterator();
  RooRealVar* anobs = NULL;

  /*
  // go about filling histograms
  RooArgSet projVars;
  for(int i = 0; i < fObsList.getSize(); i++)
  {
    RooRealVar* obs = dynamic_cast<RooRealVar*>(fObsList.at(i));
    if(obs){
      TString varName(var.GetName());
      TString obsName(obs->GetName());
      if(varName != obsName){
        bool addObs = true;
        for(std::set<TString>::iterator sepIter = fSeparation.begin();
	    sepIter != fSeparation.end(); sepIter++){
          TString sepName(sepIter->Data());
          sepName.ToLower();
          if(sep != sepName && obsName.Contains(sepName.Data()))
            addObs = false;
        }
        if(addObs)
          projVars.add(*obs);
      }
    }
  }
  */

  // was incompatible with other methods --> updated load_result method of exosensitivity to write a new result object -- 20170525
  RooFitResult *result = dynamic_cast<RooFitResult*>(wsp->genobj("result"));
  // possibility of loading from different files exists...

  if (fUserEnergyCut.IsNull()){ // then do it like we have been 
    while((anobs = dynamic_cast<RooRealVar*>(piter->Next()) )){
      if ( (TString(var.GetName()) != anobs->GetName() )     ) 
	projVars.add(*anobs);
    }
    delete pdfObs;
    
    // make subtract data histo
    SetResultWspPars(result,wsp,false); // set all but named parameter to fit values and param to 0

    // prep for subtraction
    pdf->fillHistogram(mcHisto,RooArgList(var),1.0,&projVars,kTRUE,0,kFALSE);
    mcHisto->Scale(pdf->expectedEvents(RooArgList(var))/mcHisto->Integral());
    mcHisto->Sumw2();
    data->fillHistogram(dataHisto,RooArgList(var));
    dataHisto->Sumw2();
    dataHisto->Add(mcHisto,-1);

    // make mc with pdf only
    mcHisto->Reset("ICES"); //resets only Integral, Contents , Errors and Statistics
    SetResultWspPars(result,wsp,true);
    if (name.Contains("ss"))
      {
	((RooRealVar*)pdf->getVariables()->find("beta_scale"))->Print();
	((RooRealVar*)pdf->getVariables()->find("beta_scale"))->setVal(1.0);
	((RooRealVar*)pdf->getVariables()->find("beta_scale"))->Print();
      }
    pdf->fillHistogram(mcHisto,RooArgList(var),1.0,&projVars,kTRUE,0,kFALSE);
    mcHisto->Scale(pdf->expectedEvents(RooArgList(var))/mcHisto->Integral());
    mcHisto->Sumw2();

    // here just to make clearer same block,
    wspFile.Close();
    return true;
  } else { // when we have a lb energy cut, need to project onto 2D first
    //get other vars to project onto var, energyvar
    while((anobs = dynamic_cast<RooRealVar*>(piter->Next()) )){
      if ( (TString(var.GetName())   != anobs->GetName() ) and
	   (TString(eVar->GetName()) != anobs->GetName() )     ) 
	projVars.add(*anobs); //check if clone right?
    }
    delete pdfObs;
    projVars.Print();

    bool isE = (TString(var.GetName()) == eVar->GetName() ); // need tweaks when energy is var,

    RooAbsBinning& eBinning = eVar->getBinning();
    TH2D* mc2d = new TH2D ("mc2d","",binning.numBins(),binning.array(),eBinning.numBins(),eBinning.array());
    TH2D* mc2ds= new TH2D ("mc2ds","sub mc",binning.numBins(),binning.array(),eBinning.numBins(),eBinning.array());
    TH2D* da2d = new TH2D ("da2d","",binning.numBins(),binning.array(),eBinning.numBins(),eBinning.array());

    RooArgList varList (var,*eVar);
    //fill 2d mc and data

    mcHisto->Sumw2();
    dataHisto->Sumw2();

    // do data - fit
    SetResultWspPars(result,wsp,false); // set all but named parameter to fit values and param to 0

    // prep for subtraction
    pdf->fillHistogram(mc2ds,varList,1.0,&projVars,kTRUE,0,kFALSE);
    if (isE) { // due to mechanics of find used by fillhistogram --> constant along Y
      for (int ix = 0 , bmax = eBinning.numBins() + 2 ; ix < bmax ; ix++){ 
	for (int iy = 0 ; iy < ix ; iy++){ // so set all non diagonal elements to zero.
	  mc2ds->SetBinContent(ix,iy,0);
	  mc2ds->SetBinContent(iy,ix,0);
	}
      }
      mc2ds->Scale(pdf->expectedEvents(RooArgSet(var))/mc2ds->Integral());
    } else {
      mc2ds->Scale(pdf->expectedEvents(RooArgSet(varList))/mc2ds->Integral());
    }
    mc2ds->SetEntries(mc2ds->Integral());
    mc2ds->Sumw2(); // has errors computed -- integral ok, entries off
    //mc2ds->SaveAs("mc2ds-s.C");
    // do subtraction
    data->fillHistogram(da2d,varList); // comes with errors computed
    //    da2d->SaveAs("da2d_0.C");
    da2d->Sumw2();
    da2d->Add(mc2ds,-1); // looks reasonable 
    //da2d->SaveAs("da2d-e.C"); 

    // make mc with pdf only
    //mc2d->Reset("ICES"); //resets only Integral, Contents , Errors and Statistics --> dont do this...
    SetResultWspPars(result,wsp,true);
    if (name.Contains("ss")){
      ((RooRealVar*)pdf->getVariables()->find("beta_scale"))->Print();
      ((RooRealVar*)pdf->getVariables()->find("beta_scale"))->setVal(1.0);
      ((RooRealVar*)pdf->getVariables()->find("beta_scale"))->Print();
    }
    pdf->fillHistogram(mc2d,varList,1.0,&projVars,kTRUE,0,kFALSE);
    if (isE) { // due to mechanics of find used by fillhistogram --> constant along Y
      for (int ix = 0 , bmax = eBinning.numBins() + 2 ; ix < bmax ; ix++){ 
	for (int iy = 0 ; iy < ix ; iy++){ // so set all non diagonal elements to zero.
	  mc2d->SetBinContent(ix,iy,0);
	  mc2d->SetBinContent(iy,ix,0);
	}
      }
      mc2d->Scale(pdf->expectedEvents(RooArgSet(var))/mc2d->Integral());
    } else {
      mc2d->Scale(pdf->expectedEvents(RooArgSet(varList))/mc2d->Integral());
    }
    // no errors here... when using that reset. split to two hist.
    mc2d->SetEntries(mc2d->Integral());
    mc2d->Sumw2();
    //mc2d->SaveAs("mc2d.C");

    // then do projections over ranges 
    std::vector<std::string> rngs =
      EXOFittingUtil::ReadSeparatedByChar(fUserEnergyCut.Data(),':');
    int nrngs = rngs.size();
    for (int jx = 0 ; jx < nrngs ; jx ++ ) {
      std::vector<std::string> rng = EXOFittingUtil::ReadSeparatedByChar(rngs.at(jx).c_str(),',');
      if (rng.size() != 2 ) 
	LogEXOMsg(Form("UserEnergyCut range parsing failure %s",rngs.at(jx).c_str()), EEAlert);
      double lo = atof (rng.at(0).c_str()) ; 
      double hi = atof (rng.at(1).c_str()) ; 
      if (lo > hi ) LogEXOMsg(Form("UserEnergyCut range parsing failure %s",
				   rngs.at(jx).c_str()), EEAlert);
       // else select some bins
      int lob = eBinning.rawBinNumber(lo);
      int hib = eBinning.rawBinNumber(hi);
      TH1D* mcX = mc2d->ProjectionX("mc2d_px",lob,hib,"e");
      TH1D* daX = da2d->ProjectionX("da2d_px",lob,hib,"e");
      // mcX->SaveAs("mcX.C");
      // daX->SaveAs("daX.C");
      mcHisto->Add(mcX);
      dataHisto->Add(daX);
      std::cout<<Form("Added LB energy range from %f to %f",
		      eBinning.binLow(lob),eBinning.binHigh(hib))<<std::endl;
    }
    // mcHisto ok here
    // new TCanvas("b");
    // mcHisto->DrawCopy("e");
    delete mc2d;
    delete da2d;
  }
  mcHisto->SaveAs("mcH.C");
  dataHisto->SaveAs("daH.C");
  wspFile.Close();
  return true;
}

bool EXOSourceAgreement1D::FillSmearedMCHisto1D 
( TH1D& histo, double* vars, RooRealVar* energy, const std::string& channel, double* energies,
  double* weights, int length, int multiplicity, long int seconds, int nano, int binMC){
  // energy smearing for mc from trees
  if(not energy)
    return false;

  // split values into histo bins
  std::vector<std::vector<double> > vs(histo.GetNbinsX());
  std::vector<std::vector<double> > ws(histo.GetNbinsX());
  std::cout << "Bins = " << histo.GetNbinsX() << " " << histo.GetNbinsX() << std::endl;
  for(int i = 0; i < length; i++)
  {
    int b = histo.FindBin(vars[i]);
    if(0 < b && b <= histo.GetNbinsX())
    {
      vs[b-1].push_back(energies[i]);
      ws[b-1].push_back(weights[i]);
    }
  }

  // fill energy histogram to cut on smeared energy
  std::cout << " Energies " << energy->getMin() << " " << energy->getMax() << std::endl;

  TH1D hAux("haux","",1,energy->getMin(),energy->getMax());
  for(int b = 1; b <= histo.GetNbinsX(); b++)
  {
    hAux.Reset();
    if(not vs[b-1].empty())
      fResolution->FillSmearedMCHisto1D(hAux,channel,&vs[b-1][0],&ws[b-1][0],
					vs[b-1].size(),multiplicity,seconds,nano,binMC);
    histo.SetBinContent(b,hAux.Integral());
  }
  return true;
}

void EXOSourceAgreement1D::DeleteHistos1D(std::map<TString, TH1D*>& histos)
{
  if(not histos.empty())
  {
    for(std::map<TString, TH1D*>::iterator histo = histos.begin(); histo != histos.end(); histo++)
      delete histo->second;
    histos.clear();
  }
}

