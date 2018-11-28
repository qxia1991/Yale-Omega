#include "EXOSourceAgreementManager1D.hh"
#include "EXOFittingUtil.hh" //using readseparatedbychar -- check working with other compiles.
//#include "EXOFitInfo.hh"

ClassImp(EXOSourceAgreementManager1D)

EXOSourceAgreementManager1D::EXOSourceAgreementManager1D(const char* sourceRunsInfoFileName, const char* filename, const char* wspname, const char* productionfile)
: fRunsInfo(sourceRunsInfoFileName), fWspFileName(filename), fWspName(wspname), fProductionFilename(productionfile)
{
  Init();
}

EXOSourceAgreementManager1D::EXOSourceAgreementManager1D(EXOSourceRunsPolishedInfo& sourceRunsInfo, const char* filename, const char* wspname, const char* productionfile)
  : fRunsInfo(sourceRunsInfo), fWspFileName(filename), fWspName(wspname), fProductionFilename(productionfile)
{
  Init();
}

void EXOSourceAgreementManager1D::SetUseNoisePeriods(bool useNoise)
{
    fUseNoisePeriods=useNoise;
}

bool EXOSourceAgreementManager1D::GetUseNoisePeriods(void)
{
    return fUseNoisePeriods;
}

void EXOSourceAgreementManager1D::Init()
{
  fResultDir = "./";
  
  fSrcAgrees.clear();

  if(not ReadAvailableProductions())
    CreateAvailableProductions();
  //SetProduction("2016_Xe134_Average_fv_162_10_182");
  SetProduction("2016_Xe134_Average_fv_162_10_182_with3dcut");
  SetUseNoisePeriods(true); 

  fUserObsLimitsCut = "";
  fUserEnergyCut  = "";

  // default run selections (has purity cut in it)
  // also includes a weeklivetime cut, DQ cut, Manual Flag cut (for Low Field Runs)
  fRunsInfo.SelectDefaultRuns();
  
  // only use physics (prescale 0) trigger
  // no more 50 Hz trigger 
  std::vector<std::string> triggers;
  triggers.push_back("0");
  fRunsInfo.CutExactList("TriggerPrescale",triggers);
  
  //Intialize this to have no extra S2/S8 correction
  SetExtraCalib(1.0, 0.0, 1.0, 0.0);

  //discard Cs137 runs
  fRunsInfo.CutExact("SourceName","Cs-137",false);
 
  //S11 runs that have unmarked offset (should be S0 positon)
  fRunsInfo.CutExact("RunNumber","4095",false); //Something off with the physics trigger (sTh too I think?)
  fRunsInfo.CutExact("RunNumber","5051",false); //Th228 S11 + 2&20/32"
  fRunsInfo.CutExact("RunNumber","5053",false); //Th228 S11 - 2&20/32"
  fRunsInfo.CutExact("RunNumber","5067",false); //Co60  S11 + 3.75"
  fRunsInfo.CutExact("RunNumber","5068",false); //Co60  S11 + 1"
  fRunsInfo.CutExact("RunNumber","5069",false); //Co60  S11 - 1.75"
 
  //Phase2 Offset runs
  //fRunsInfo.CutExact("RunNumber","7502",false);
  //fRunsInfo.CutExact("RunNumber","7491",false);
  //fRunsInfo.CutExact("RunNumber","7503",false);
  //fRunsInfo.CutExact("RunNumber","7504",false);

  //Can now pass such extra cut exacts on command line via python parser to UsrCutExact
  //fRunsInfo.CutExact("SourceName","Co-60");
  //fRunsInfo.CutExact("SourcePositionS","5");

  // add standard observables
  if(not ReadWspObservables())
  {
    std::cout << "Unable to get the workspace so using default things" << std::endl;
    RooRealVar energy_ss("energy_ss","Energy",300,3500,"keV");
    energy_ss.setBins(160);
    RooRealVar energy_ms("energy_ms","Energy",300,3500,"keV");
    energy_ms.setBins(160);
    RooRealVar standoff_distance("standoff_distance","Standoff Distance",0,200,"mm");
    standoff_distance.setBins(20);
    
    fObsList.addClone(energy_ss);
    fObsList.addClone(energy_ms);
    fObsList.addClone(standoff_distance);
  }
  else
  {
    AddLBWspPdf("bb2n"); // add bb2n by default if wsp is given
  }

  std::cout << "Obs list set to: ";
  fObsList.Print();

  SetStats(15); // All legends on by default
}

EXOSourceAgreementManager1D::~EXOSourceAgreementManager1D()
{
  fProductions.clear();
  fSrcAgrees.clear();
}

bool EXOSourceAgreementManager1D::ReadWspObservables()
{
  TFile wspFile(fWspFileName.Data(),"read");
  if(wspFile.IsZombie())
  {
    fWspFileName = "";
    fWspName = "";
    return false;
  }
  RooWorkspace* wsp = dynamic_cast<RooWorkspace*>(wspFile.Get(fWspName.Data()));
  if(not wsp)
  {
    fWspFileName = "";
    fWspName = "";
    return false;
  }

  std::set<std::string> allObs;

  EXOFitInfo* fitInfo = dynamic_cast<EXOFitInfo*>(wsp->genobj("FitInfo"));
  if(fitInfo)
  {
    allObs = fitInfo->GetAllObservables();
  }
  else
  {    
    std::list<RooAbsData*> allData = wsp->allData();
    for(std::list<RooAbsData*>::iterator data = allData.begin(); data != allData.end(); data++)
    {
      const RooArgSet* obsSet = (*data)->get();
      TIterator *obsIter = obsSet->createIterator();
      RooRealVar *obs = NULL;
      while((obs = dynamic_cast<RooRealVar*>(obsIter->Next())))
      {
        allObs.insert(obs->GetName());
      }
      delete obsIter;
    }
  }

  fObsList.removeAll();
  for(std::set<std::string>::iterator obsName = allObs.begin(); obsName != allObs.end(); obsName++)
  {
    //fObsList.addClone(*wsp->var(obsName->c_str()));
    std::cout << "Adding var=" << (obsName->c_str())  << std::endl;
    
    //something odd related to fObsList.isOwning()
    //fObsList.addClone(*wsp->var(obsName->c_str()));
    fObsList.add(*wsp->var(obsName->c_str()));
    
    std::cout << "Success"  << std::endl;
  }
  
  std::cout << " Separation type " << fitInfo->GetSeparationMethod() << std::endl;
  
  if (fitInfo->GetSeparationMethod() == EXOFittingUtil::kNone) {
     std::cout << " USE ALL " << fitInfo->GetSeparationMethod() << std::endl;
     fSepType=EXOFittingUtil::kNone;
  }
  else if(fitInfo->GetSeparationMethod() == EXOFittingUtil::kSSMS){
      std::cout << " USE SS/MS " << fitInfo->GetSeparationMethod() << std::endl;
     fSepType=EXOFittingUtil::kSSMS;
  }
  else {
    std::cout << " FAIL " << fitInfo->GetSeparationMethod() << std::endl;
    fSepType=1;
  }
  
  wspFile.Close();
  return true;
}

bool EXOSourceAgreementManager1D::LimitObs(std::vector<std::string> limitTo){
  // in order to only do source agreement for a single variable of a workspace
  // this is now functional, implements through limiting the pairing of observable indices and separation strings.

  limObs.clear();
  for (std::vector<std::string>::iterator vname = limitTo.begin();
       vname != limitTo.end(); vname++){
    for(int o = 0; o < (int) fObsList.getSize(); o++){
      RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(o));
      if(not var) continue;
      TString varName(var->GetName());
      if(varName.CompareTo(*vname) == 0 )
	limObs.insert( o ) ;
    }
  }
  if (limObs.size() == 0 ) return false ;

  std::cout << "Obs list limited to : ";
  for(std::set<int>::iterator x = limObs.begin(); x!= limObs.end(); x++){
    std::cout << fObsList.at(*x) ; }
  std::cout << std::endl;

  return true;
}


bool EXOSourceAgreementManager1D::AddLBWspPdf(const char* pdfname)
{
  TString pdfName(pdfname);
  
  if(fWspFileName == "" or fWspName == "")
    return false;

  TFile wspFile(fWspFileName.Data(),"read");
  RooWorkspace* wsp = dynamic_cast<RooWorkspace*>(wspFile.Get(fWspName.Data()));
  RooRealVar* num_pdf = dynamic_cast<RooRealVar*>(wsp->var(Form("num_%s",pdfName.Data())));
  if(not num_pdf)
    return false;

  std::cout << "Adding Pdf: " << pdfName.Data() << std::endl;
  fPdfNames.insert(pdfName.Data());
  return true;
}

const char* EXOSourceAgreementManager1D::GetSourceKey(const char* name, const char* position)
{
  return Form("%s_S%s",name,position);
}

std::pair<TString, TString> EXOSourceAgreementManager1D::BreakKey(const char* key)
{
  TString source(key);
  TString pos(key);

  source.Resize(source.First("_"));
  pos.ReplaceAll(Form("%s_S",source.Data()),"");

  return std::make_pair(source,pos);
}

bool EXOSourceAgreementManager1D::SetProduction(TString name)
{
  if(fProductions.count(name) <= 0)
    return false;
  fProduction = name;
  return true;
}

bool EXOSourceAgreementManager1D::SetStats(Byte_t stats)
{
  fStats = stats;
  return true;
}

bool  EXOSourceAgreementManager1D::UsrCutExact(std::string ucut){
  std::cout << "Applying User Cut : " << ucut <<std::endl;
  std::vector<std::string> opts =
    EXOFittingUtil::ReadSeparatedByChar(ucut.c_str(),':');
  if (opts.size() < 2) return false;
  bool keep = true ;
  if (opts.size() >= 3 )
    if (opts.at(2) == "0" or opts.at(2) == "False" or opts.at(2) == "false")
      keep = false;
  std::cout << "Using Exact Cut " << opts.at(0) << " " << opts.at(1) << " " << keep << std::endl;
  fRunsInfo.CutExact(opts.at(0),opts.at(1),keep);
  return true;
}
bool  EXOSourceAgreementManager1D::UsrCutDoubleComparison(std::string ucut){
  std::cout << "Applying User Cut : " << ucut <<std::endl;
  std::vector<std::string> opts =
    EXOFittingUtil::ReadSeparatedByChar(ucut.c_str(),':');
  if (opts.size() < 2) return false;
  double anumber = std::atof( (opts.at(1)).c_str() ) ; //stod is only in c++11 +
  bool keep = true ;
  if (opts.size() >= 3 )
    if (opts.at(2) == "0" or opts.at(2) == "False" or opts.at(2) == "false")
      keep = false;
  std::cout << "Using Double Cut " << opts.at(0) << " " << anumber << " " << keep << std::endl;
  fRunsInfo.CutDoubleComparison(opts.at(0),anumber,keep);
  return true;
}

bool EXOSourceAgreementManager1D::RunAgreement(TString dir)
{
  //core action -- crease source agreements for source positions and LB then process
  CreateAvailableSourceAgreements();
  AddRequestedLBAgreements();
  
  if (! dir.IsNull()) fResultDir = dir; // trust init to set ./ , 
  //allows for arg less running if interactive init sets the dir

  for(std::map<TString, EXOSourceAgreement1D>::iterator source = fSrcAgrees.begin(); source != fSrcAgrees.end(); source++)
  {
    TString name = source->first;
    std::cout << "Running agreement for key : " << name << " ..." << std::endl;
    EXOSourceAgreement1D* agree = &(source->second);

    if(name.Contains("S2") or name.Contains("S8")){
      std::cout << "Using Caio's Calibrations for S2/S8" << std::endl;
      //Phase1: agree->SetExtraCalib(1.0076, -10.0, 1.0085, -6.0);
      //Phase2: agree->SetExtraCalib(1.0038, -2.0, 1.0029, -2.0);
      agree->SetExtraCalib(aSS, bSS, aMS, bMS);
    } else  {
      // Don't use extra calib 
      // calibration is E*a + b so this will set b=0 and a=1
      agree->SetExtraCalib(1.0, 0.0, 1.0, 0.0);
    }

    agree->FillHistos();
    agree->EvalShape();
    agree->EvalSSFractions();
    
    //individual agreement file saves : 
    agree->SaveToFile(Form("%s/SourceAgreement_%s_%s.root",
			   fResultDir.Data(),fProduction.Data(),name.Data() ) );
  }


  // then digest and make prettier plots with multiple th1 components
  std::set<TString> sources;
  std::set<TString> positions;

  for(std::map<TString, EXOSourceAgreement1D>::iterator source = fSrcAgrees.begin();
      source != fSrcAgrees.end(); source++){
    // agreement for each type and position
    std::pair<TString, TString> sp = BreakKey(source->first.Data());
    sp.second = Form("S%s",sp.second.Data());
    
    if(sources.count(sp.first.Data()) == 0)
    {
      DigestInfoForAll(sp.first.Data());
      sources.insert(sp.first.Data());
    }

    if(positions.count(sp.second.Data()) == 0)
    {
      DigestInfoForAll(sp.second.Data());
      positions.insert(sp.second.Data());
    }
  }

  DigestInfoForAll("_"); // for everything.
  
  std::cout << "Sucess" << std::endl;

  return true;
}

bool EXOSourceAgreementManager1D::DigestInfoForAll(TString ref)
{
  std::cout << "Digesting info for all " << ref << " ... " << std::endl;
  
  std::map<TString, TCanvas*> canvasList;
  std::map<TString, TLegend*> legendList;
  
  for(std::map<TString, EXOSourceAgreement1D>::iterator source = fSrcAgrees.begin(); source != fSrcAgrees.end(); source++)
  {
    TString sourceKey = source->first;
    if(not sourceKey.Contains(ref.Data()))
      continue;
    
    int color = GetSourceColor(sourceKey.Data());
    int marker = GetSourceMarker(sourceKey.Data());
    
    EXOSourceAgreement1D* agree = &(source->second);
    
    for(std::map<TString, std::vector<TString> >::const_iterator iter = agree->GetAgreements()->begin(); iter != agree->GetAgreements()->end(); iter++)
    {
      TString agreeName = iter->first;
      const std::vector<TString>* agreeObs = &(iter->second);
      for(std::vector<TString>::const_iterator obs = agreeObs->begin(); obs != agreeObs->end(); obs++)
      {
        if(agree->GetComparisons()->count("data") > 0 and agree->GetComparisons()->count("mc") > 0)
        {
          TH1D* data = dynamic_cast<TH1D*>(agree->GetResultHisto(agreeName.Data(),obs->Data(),"data"));
          TH1D* mc = dynamic_cast<TH1D*>(agree->GetResultHisto(agreeName.Data(),obs->Data(),"mc"));

          if(not (data and mc))
            continue;
          if(data->GetEntries() == 0 or mc->GetEntries() == 0)
            continue;

          TString drawSame = "same";
          TString canvasName(Form("%s_%s_spect",agreeName.Data(),obs->Data()));
          if(canvasList.count(canvasName.Data()) <= 0)
          {
            canvasList.insert(std::make_pair(canvasName.Data(),new TCanvas(canvasName.Data())));
            legendList.insert(std::make_pair(canvasName.Data(),new TLegend(0.1,0.7,0.48,0.9)));
            drawSame = "";
          }
          TCanvas* canvas = canvasList.at(canvasName.Data());
          TLegend* legend = legendList.at(canvasName.Data());


          canvas->cd();
          data->SetMarkerColor(color);
          data->SetMarkerStyle(marker);
          data->SetMarkerSize(0.7);
          data->SetLineColor(color);
          data->GetXaxis()->SetTitle(obs->Data());
          data->GetYaxis()->SetTitle("Normalized events (a.u.)");
          mc->SetLineColor(color);
          mc->SetLineWidth(2);
          mc->GetXaxis()->SetTitle(obs->Data());
          if(obs->Contains("energy"))
          {
            for(int o = 0; o < fObsList.getSize(); o++)
            {
              RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(o));
              if(not var)
                continue;
              TString varName(var->GetName());
              if(varName.Contains("energy"))
              {
                TString spacing(" ");
                if(obs->Contains("energy_ss"))
                  spacing = " SS ";
                if(obs->Contains("energy_ms"))
                  spacing = " MS ";
                mc->GetXaxis()->SetTitle(Form("%s%s(%s)",var->GetTitle(),spacing.Data(),var->getUnit()));
                break;
              }
            }
          }
          else
          {
            TString varName(obs->Data());
            RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.find(varName.Data()));
            if(not var)
            {
              varName.Resize(varName.Last('_'));
              var = dynamic_cast<RooRealVar*>(fObsList.find(varName.Data()));
            }
            if(var)
              mc->GetXaxis()->SetTitle(Form("%s (%s)",var->GetTitle(),var->getUnit()));
          }
          mc->GetYaxis()->SetTitle("Normalized events (a.u.)");
          mc->Draw(Form("hist %s",drawSame.Data()));
          data->Draw("e same"); //Form("e %s",drawSame.Data()));
          legend->AddEntry(data,sourceKey.Data(),"PL");
        }
        
        for(std::set<TString>::const_iterator comp = agree->GetComparisons()->begin(); comp != agree->GetComparisons()->end(); comp++)
        {
          if(*comp == "data" or *comp == "mc")
            continue;

          TString drawSame = "same";
          TString canvasName(Form("%s_%s_%s",agreeName.Data(),obs->Data(),comp->Data()));
          if(canvasList.count(canvasName.Data()) <= 0)
          {
            canvasList.insert(std::make_pair(canvasName.Data(),new TCanvas(canvasName.Data())));
            legendList.insert(std::make_pair(canvasName.Data(),new TLegend(0.1,0.7,0.48,0.9)));
            drawSame = "";
          }
          TCanvas* canvas = canvasList.at(canvasName.Data());
          TLegend* legend = legendList.at(canvasName.Data());

          TH1D* histo = dynamic_cast<TH1D*>(agree->GetResultHisto(agreeName.Data(),obs->Data(),comp->Data()));

          canvas->cd();
          histo->SetMarkerColor(color);
          histo->SetMarkerStyle(marker);
          histo->SetMarkerSize(0.7);
          histo->SetLineColor(color);
          histo->GetXaxis()->SetTitle(obs->Data());
          if(obs->Contains("energy"))
          {
            for(int o = 0; o < fObsList.getSize(); o++)
            {
              RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(o));
              if(not var)
                continue;
              TString varName(var->GetName());
              if(varName.Contains("energy"))
              {
                TString spacing(" ");
                if(obs->Contains("energy_ss"))
                  spacing = " SS ";
                if(obs->Contains("energy_ms"))
                  spacing = " MS ";
                histo->GetXaxis()->SetTitle(Form("%s%s(%s)",var->GetTitle(),spacing.Data(),var->getUnit()));
                break;
              }
            }
          }
          else
          {
            TString varName(obs->Data());
            RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.find(varName.Data()));
            if(not var)
            {
              varName.Resize(varName.Last('_'));
              var = dynamic_cast<RooRealVar*>(fObsList.find(varName.Data()));
            }
            if(var)
            {
              histo->GetXaxis()->SetTitle(Form("%s (%s)",var->GetTitle(),var->getUnit()));
            }
          }
          histo->GetYaxis()->SetTitle(Form("Comparison %s",comp->Data()));
          histo->Draw(Form("e %s",drawSame.Data()));
          legend->AddEntry(histo,sourceKey.Data(),"PL");
        }
      }
    }
    
  }
  
  TFile outFile(Form("%s/DigestedSourceAgreement_%s_%s.root",fResultDir.Data(),fProduction.Data(),(ref == "_" ? "All" : ref.Data())),"recreate");
  for(std::map<TString, TCanvas*>::iterator canvasIter = canvasList.begin(); canvasIter != canvasList.end(); canvasIter++)
  {
    outFile.cd();
    TCanvas* canvas = canvasIter->second;
    TString canvasName = canvasIter->first;
    
    if(canvas and legendList.count(canvasName) > 0)
    {

      TLegend* legend = legendList.at(canvasName);
      canvas->cd();
      legend->SetName("PlotLegend");
      if(legend && ((fStats & 1) != 0))
        legend->Draw();

      TLegend* avgLegend = GetLegendOfAverages(legend);
      canvas->cd();
      if(avgLegend && ((fStats & 2) != 0))
        avgLegend->Draw();

      TLegend* corLegend = GetLegendOfCorrelations(legend);
      canvas->cd();
      if(corLegend && ((fStats & 4) != 0))
        corLegend->Draw();

      TLegend* wCorLegend = GetLegendOfWeightedCorrelations(legend);
      canvas->cd();
      if(wCorLegend && ((fStats & 8) != 0))
        wCorLegend->Draw();
      
      canvas->Write();
      delete canvas;
      if(avgLegend)
        delete avgLegend;
      if(corLegend)
        delete corLegend;
      if(wCorLegend)
        delete wCorLegend;
    }
  }
  outFile.Close();

  return true;
}

TLegend* EXOSourceAgreementManager1D::GetLegendOfAverages(TLegend* legend)
{
  if(not legend)
    return 0;

  TLegend* result = new TLegend(0.1,0.7,0.48,0.9);
  result->SetName("AverageLegend");
  result->SetNColumns(4);
  result->SetMargin(0);
  result->SetTextAlign(22);
  result->SetHeader("Weighted Averages");
  
  TList* legEntries = legend->GetListOfPrimitives();
  double totSum = 0.;
  double totNorm = 0.;
  double totAbs = 0.;
  for(int i = 0; i < legEntries->GetSize(); i++)
  {
    TLegendEntry *legEntry = dynamic_cast<TLegendEntry*>(legEntries->At(i));
    if(not legEntry)
      continue;
    TH1D *histo = dynamic_cast<TH1D*>(legEntry->GetObject());
    if(not histo)
      continue;
    double histSum = 0.;
    double histNorm = 0.;
    double histAbs = 0.;
    for(int b = 1; b <= histo->GetNbinsX(); b++)
    {
      double content = histo->GetBinContent(b);
      double weight = histo->GetBinError(b);
      if(weight > 0)
      {
        weight *= weight;
        weight = 1/weight;
        histSum += content*weight;
        histNorm += weight;
        histAbs += weight*fabs(content);
      }
    }

    double histAvg = histNorm > 0 ? histSum/histNorm : 0;
    double histAbsAvg = histNorm > 0 ? histAbs/histNorm : 0;
    double histErr = histNorm > 0 ? sqrt(1./histNorm) : 0;
    result->AddEntry((TObject*)0,legEntry->GetLabel(),"");
    result->AddEntry((TObject*)0,Form("%0.4f",histAvg),"");
    result->AddEntry((TObject*)0,Form("%0.4f",histAbsAvg),"");
    result->AddEntry((TObject*)0,Form("#pm %.4f",histErr),"");
    
    totSum += histSum;
    totNorm += histNorm;
    totAbs += histAbs;
  }

  double totAvg = totNorm > 0 ? totSum/totNorm : 0.;
  double totAbsAvg = totNorm > 0 ? totAbs/totNorm : 0.;
  double totErr = totNorm > 0 ? sqrt(1./totNorm) : 0.;
  result->AddEntry((TObject*)0,"Total","");
  result->AddEntry((TObject*)0,Form("%0.4f",totAvg),"");
  result->AddEntry((TObject*)0,Form("%0.4f",totAbsAvg),"");
  result->AddEntry((TObject*)0,Form("#pm %.4f",totErr),"");

  return result;
}

TLegend* EXOSourceAgreementManager1D::GetLegendOfCorrelations(TLegend* legend)
{
  if(not legend)
    return 0;

  TLegend* result = new TLegend(0.1,0.7,0.48,0.9);
  result->SetName("CorrelationLegend");
  result->SetMargin(0);
  result->SetTextAlign(22);
  result->SetHeader("Correlations");

  std::map<TString, TH1D*> histos;
  TList* legEntries = legend->GetListOfPrimitives();
  for(int i = 0; i < legEntries->GetSize(); i++)
  {
    TLegendEntry *legEntry = dynamic_cast<TLegendEntry*>(legEntries->At(i));
    if(not legEntry)
      continue;
    TH1D *histo = dynamic_cast<TH1D*>(legEntry->GetObject());
    if(not histo)
      continue;
    histos.insert(std::make_pair(legEntry->GetLabel(),histo));
  }

  result->SetNColumns(histos.size()+1);
  result->AddEntry((TObject*)0,"","");

  std::map<TString, double> averages;
  std::map<TString, double> stddevs;
  for(std::map<TString, TH1D*>::iterator histo = histos.begin(); histo != histos.end(); histo++)
  {
    TString hName = histo->first;
    TH1D* hHisto = histo->second;
    
    result->AddEntry((TObject*)0,hName.Data(),"");
    hHisto->Fit("pol0","QW0");
    double average = hHisto->GetFunction("pol0")->GetParameter(0);
    averages[hName] = average;
    stddevs[hName] = 0.;
    for(int b = 1; b <= hHisto->GetNbinsX(); b++)
    {
      double content = hHisto->GetBinContent(b);
      stddevs[hName] += (content - average) * (content - average);
    }
    //I think a 1/N is missing here??
    stddevs[hName] = sqrt(stddevs[hName]);
  }

  for(std::map<TString, TH1D*>::iterator hrIter = histos.begin(); hrIter != histos.end(); hrIter++)
  {
    TString hrName = hrIter->first;
    TH1D* hrHisto = hrIter->second;
    double hrAvg = averages.at(hrName);
    double hrStdDev = stddevs.at(hrName);

    result->AddEntry((TObject*)0,hrName.Data(),"");

    for(std::map<TString, TH1D*>::iterator hcIter = histos.begin(); hcIter != histos.end(); hcIter++)
    {
      TString hcName = hcIter->first;
      TH1D* hcHisto = hcIter->second;
      double hcAvg = averages.at(hcName);
      double hcStdDev = stddevs.at(hcName);

      double correlation = 0.;
      for(int b = 1; b <= hcHisto->GetNbinsX(); b++)
        correlation += (hrHisto->GetBinContent(b) - hrAvg) * (hcHisto->GetBinContent(b) - hcAvg);
      //Also think a 1/N is missing here for E[(x-ux)*(y-uy)]
      correlation /= (hrStdDev * hcStdDev);

      result->AddEntry((TObject*)0,Form("%0.3f",correlation),"");
    }
  }

  return result;
}

TLegend* EXOSourceAgreementManager1D::GetLegendOfWeightedCorrelations(TLegend* legend)
{
  // weighted correlation use weight given by max error of two compared histograms
  // thus the actual correlation depends on the two histograms...
  
  if(not legend)
    return 0;

  TLegend* result = new TLegend(0.1,0.7,0.48,0.9);
  result->SetName("WeightedCorrelationLegend");
  result->SetMargin(0);
  result->SetTextAlign(22);
  result->SetHeader("Weighted Correlations");

  std::map<TString, TH1D*> histos;
  TList* legEntries = legend->GetListOfPrimitives();
  for(int i = 0; i < legEntries->GetSize(); i++)
  {
    TLegendEntry *legEntry = dynamic_cast<TLegendEntry*>(legEntries->At(i));
    if(not legEntry)
      continue;
    TH1D *histo = dynamic_cast<TH1D*>(legEntry->GetObject());
    if(not histo)
      continue;
    histos.insert(std::make_pair(legEntry->GetLabel(),histo));
  }

  result->SetNColumns(histos.size()+1);
  result->AddEntry((TObject*)0,"","");

  for(std::map<TString, TH1D*>::iterator histo = histos.begin(); histo != histos.end(); histo++)
    result->AddEntry((TObject*)0,histo->first.Data(),"");

  for(std::map<TString, TH1D*>::iterator hrIter = histos.begin(); hrIter != histos.end(); hrIter++)
  {
    TString hrName = hrIter->first;
    TH1D* hrHisto = hrIter->second;

    result->AddEntry((TObject*)0,hrName.Data(),"");

    for(std::map<TString, TH1D*>::iterator hcIter = histos.begin(); hcIter != histos.end(); hcIter++)
    {
      TString hcName = hcIter->first;
      TH1D* hcHisto = hcIter->second;

      int nBins = hcHisto->GetNbinsX();
      std::vector<double> weights(nBins);
      
      double hrAvg = 0.;
      double hcAvg = 0.;
      double hrStdDev = 0.;
      double hcStdDev = 0.;
      double norm = 0.;
      double corr = 0.;
      
      for(int b = 1; b <= nBins; b++)
      {
        double br = hrHisto->GetBinContent(b);
        double wr = hrHisto->GetBinError(b);
        double bc = hcHisto->GetBinContent(b);
        double wc = hcHisto->GetBinError(b);

        double max = std::max(wr,wc);
        double min = std::min(wr,wc);
        
        double weight = (min <= 0) ? 0 : max;
        weight = (weight > 0) ? 1./(weight*weight) : 0.;

        hrAvg += weight*br;
        hcAvg += weight*bc;
        
        norm += weight;
        weights[b-1] = weight;
      }

      if(norm > 0)
      {
        hrAvg /= norm;
        hcAvg /= norm;
      }

      for(int b = 1; b <= nBins; b++)
      {
        double br = hrHisto->GetBinContent(b);
        double bc = hcHisto->GetBinContent(b);

        hrStdDev += weights[b-1]*(br - hrAvg)*(br - hrAvg);
        hcStdDev += weights[b-1]*(bc - hcAvg)*(bc - hcAvg);
        corr += weights[b-1]*(br - hrAvg)*(bc - hcAvg);
      }

      if(norm > 0)
      {
        hrStdDev /= norm;
        hcStdDev /= norm;
        corr /= norm;
        
        hrStdDev = sqrt(hrStdDev);
        hcStdDev = sqrt(hcStdDev);
        corr /= (hrStdDev*hcStdDev);
      }

      result->AddEntry((TObject*)0,Form("%0.3f",corr),"");
    }
  }

  return result;
}

void EXOSourceAgreementManager1D::SaveToFile(const char* name)
{
  TFile file(name,"recreate");
  file.Close();
}

bool EXOSourceAgreementManager1D::CreateAvailableSourceAgreements()
{
  fSrcAgrees.clear();

  std::vector<std::string> sources = fRunsInfo.GetListOf("SourceName");
  std::vector<std::string> positions = fRunsInfo.GetListOf("SourcePositionS");

  DataMCProduction* production = &fProductions.at(fProduction);
  
  size_t n = sources.size();
  for(size_t i = 0; i < n; i++)
  {
    TString key = GetSourceKey(sources[i].c_str(),positions[i].c_str());
    if(fSrcAgrees.count(key) > 0)
      continue;

    EXOSourceRunsPolishedInfo runsInfo(fRunsInfo);
    runsInfo.CutExact("SourceName",sources[i]);
    runsInfo.CutExact("SourcePositionS",positions[i]);

    if(runsInfo.GetNRuns() == 0)
      continue;

    runsInfo.SetMetadata("DataSelectionFileName",production->fDataFiles.Data());
    runsInfo.SetMCTreeFileNames(production->fPreMCLocation.Data());
    runsInfo.SetMCSelectionFileNames(production->fCutMCname.Data(),production->fCutMCLocation.Data());
    
    std::cout << "Adding key : " << key << " to list of source agreements ..." << std::endl;
    //Add something to take in seperation things
    fSrcAgrees.insert(std::make_pair(key,EXOSourceAgreement1D(runsInfo)));

    EXOSourceAgreement1D* agree = &fSrcAgrees.at(key);
    agree->SetSepType(fSepType);
    agree->SetResolution(production->fResolFlavor,production->fResolTable,production->fResolWeekly);
    agree->SetUserObservableLimitsCut(fUserObsLimitsCut.Data());
    agree->SetUserEnergyCut(fUserEnergyCut.Data() );

    if(fUseNoisePeriods)
    {
        //This isn't completely correct since it doesn't account for missing Source Data
        //agree->SetHistogramWeights("NoiseIndex","NoiseLivetime");
        agree->SetHistogramWeights("NoiseLevel","NoiseLevelLivetime");
    }
    else
    {
        if(sources[i] == "Th-228" && positions[i] == "5")
            agree->SetHistogramWeights("WeekIndex", "WeekLivetime");
        else
        {
            if(sources[i] == "Th-228" && positions[i] == "5")
                agree->SetHistogramWeights("WeekIndex","WeekLivetime");
            else
                agree->SetHistogramWeights("CampaignIndex","CampaignLivetime");
        }
    }
    for(int j = 0; j < fObsList.getSize(); j++)
    {
      RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(j));
      if(var)
        agree->AddObservable(*var);
    }
    
    agree->PairObsToSeps(limObs);

  }
  return true;
}

bool EXOSourceAgreementManager1D::AddRequestedLBAgreements()
{
  for(std::set<TString>::iterator pdfName = fPdfNames.begin(); pdfName != fPdfNames.end(); pdfName++)
  {
    TString key = GetSourceKey(pdfName->Data(),"LB");
    if(fSrcAgrees.count(key) > 0)
      continue;
    
    std::cout << "Adding key : " << key << " to list of source agreements ..." << std::endl;
    fSrcAgrees.insert(std::make_pair(key,EXOSourceAgreement1D(fWspFileName.Data(),fWspName.Data(),pdfName->Data(),false)));

    EXOSourceAgreement1D* agree = &fSrcAgrees.at(key);
    for(int j = 0; j < fObsList.getSize(); j++)
    {
      RooRealVar* var = dynamic_cast<RooRealVar*>(fObsList.at(j));
      if(var)
        agree->AddObservable(*var,true);
    }
    agree->SetSepType(fSepType);
    agree->PairObsToSeps(limObs);
    agree->SetUserObservableLimitsCut(fUserObsLimitsCut.Data());
    agree->SetUserEnergyCut(fUserEnergyCut.Data() );
  }
  return true;
}

int EXOSourceAgreementManager1D::GetSourceColor(const char* key)
{
  std::pair<TString, TString> sp = BreakKey(key);
  TString source = sp.first;
  TString pos = sp.second;

  std::map<TString, int> colors;
  colors["Th-228"] = kAzure;
  colors["Co-60"] = kSpring;
  colors["Ra-226"] = kOrange;
  colors["Cs-137"] = kViolet;
  colors["Other"] = kPink;
  colors["bb2n"] = 0;

  if(colors.count(source) == 0)
    source = "Other";
  
  std::map<TString, int> offset;
  offset["5"] = 0;
  offset["2"] = 10;
  offset["8"] = -9;
  offset["11"] = -7;
  offset["17"] = 7;
  offset["0"] = 9;
  offset["other"] = 1;
  offset["LB"] = kBlack;

  if(offset.count(pos) == 0)
    pos = "other";
  
  return colors.at(source) + offset.at(pos);
}

int EXOSourceAgreementManager1D::GetSourceMarker(const char* key)
{
  std::pair<TString, TString> sp = BreakKey(key);
  TString source = sp.first;
  TString pos = sp.second;

  std::map<TString, int> colors;
  colors["Th-228"] = 24;
  colors["Co-60"] = 25;
  colors["Ra-226"] = 26;
  colors["Cs-137"] = 27;
  colors["bb2n"] = 28;
  colors["Other"] = 30;

  if(colors.count(source) == 0)
    source = "Other";
  
  std::map<TString, int> offset;
  offset["5"] = 0;
  offset["2"] = -4;
  offset["8"] = -4;
  offset["11"] = 0;
  offset["17"] = 0;
  offset["0"] = 5;
  offset["LB"] = 0;
  offset["other"] = 5;

  if(offset.count(pos) == 0)
    pos = "other";
  
  return colors.at(source) + offset.at(pos);
}


void EXOSourceAgreementManager1D::CreateAvailableProductions()
{
  std::cout << "Creating default production list." << std::endl;

  fProductions.clear();

  DataMCProduction *production = 0;
  
  fProductions.insert(std::make_pair("2016_Xe134_Average_fv_162_10_182",DataMCProduction("2016_Xe134","2016-v1-average","energy-mcbased-fit",false)));
  production = &fProductions.at("2016_Xe134_Average_fv_162_10_182");
  production->fDataFiles = "/nfs/slac/g/exo_data6/groups/Energy/data/WIPP/selection/2016_Xe134_newDN/AfterCalibration/fv_162_10_182/Xe134_run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/preprocessed/2015_Xe134/";
  production->fCutMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/selection/2015_Xe134/fv_162_10_182/";
  production->fCutMCname = ".cut.root";

  fProductions.insert(std::make_pair("2016_Xe134_Average_fv_162_10_182_with3dcut",DataMCProduction("2016_Xe134","2016-v1-average","energy-mcbased-fit",false)));
  production = &fProductions.at("2016_Xe134_Average_fv_162_10_182_with3dcut");
  production->fDataFiles = "/nfs/slac/g/exo_data4/users/licciard/Xe134/systs/data/WIPP/with3dcut/Xe134_run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/preprocessed/2015_Xe134/";
  production->fCutMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/selection/2015_Xe134/fv_162_10_182/";
  production->fCutMCname = ".cut.root";

  fProductions.insert(std::make_pair("2016_Xe134_Average_fv_162_10_182_no3dcut",DataMCProduction("2016_Xe134","2016-v1-average","energy-mcbased-fit",false)));
  production = &fProductions.at("2016_Xe134_Average_fv_162_10_182_no3dcut");
  production->fDataFiles = "/nfs/slac/g/exo_data4/users/licciard/Xe134/systs/data/WIPP/no3dcut/Xe134_run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/preprocessed/2015_Xe134/";
  production->fCutMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/selection/2015_Xe134/fv_162_10_182_no3dcut/";
  production->fCutMCname = ".cut.root";

  
  fProductions.insert(std::make_pair("2016_Xe134_Weekly_fv_162_10_182",DataMCProduction("2016_Xe134","2016-v1-weekly","energy-mcbased-fit",true)));
  production = &fProductions.at("2016_Xe134_Weekly_fv_162_10_182");
  production->fDataFiles = "/nfs/slac/g/exo_data6/groups/Energy/data/WIPP/selection/2016_Xe134_newDN/AfterCalibration/fv_162_10_182/Xe134_run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/preprocessed/2015_Xe134/";
  production->fCutMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/selection/2015_Xe134/fv_162_10_182/";
  production->fCutMCname = ".cut.root";

  //Mike's reprocess of the Phase1 data.  Manuel and Caio made PreProc but just Source data
  fProductions.insert(std::make_pair("2016_Xe134_Average_fv_162_10_182_Reproc",DataMCProduction("2016_Xe134","2016-v1-average","energy-mcbased-fit",false)));
  production = &fProductions.at("2016_Xe134_Average_fv_162_10_182_Reproc");
  production->fDataFiles = "/nfs/slac/g/exo_data6/groups/Energy/data/WIPP/selection/2016_Xe134_newDN_reproc_120716/AfterCalibration/fv_162_10_182/Xe134_run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/preprocessed/2015_Xe134/";
  production->fCutMCLocation = "/nfs/slac/g/exo_data6/groups/Fitting/data/MC/selection/2015_Xe134/fv_162_10_182/";
  production->fCutMCname = ".cut.root";

  //Phase II check
  fProductions.insert(std::make_pair("2017_PhaseII_fv_162_10_182_Reproc",DataMCProduction("2017_PhaseII","2016-v3-average","energy-mcbased-fit",false)));
  production = &fProductions.at("2017_PhaseII_fv_162_10_182_Reproc");
  production->fDataFiles = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NewLightMap_2_14_2017/selection/run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NewLightMap_2_14_2017/mc/";
  production->fCutMCLocation = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NewLightMap_2_14_2017/mc/";
  production->fCutMCname = ".cut.root";

  //Phase II check 2
  fProductions.insert(std::make_pair("2017_PhaseII_fv_162_10_182_Reproc2",DataMCProduction("2017_PhaseII","2016-v3-average","energy-mcbased-fit",false)));
  production = &fProductions.at("2017_PhaseII_fv_162_10_182_Reproc2");
  production->fDataFiles = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NewLightMap_3_4_2017/selection/run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NewLightMap_3_4_2017/mc/";
  production->fCutMCLocation = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NewLightMap_3_4_2017/mc/";
  production->fCutMCname = ".cut.root";
  
  //Phase II check Noise
  fProductions.insert(std::make_pair("2017_PhaseII_fv_162_10_182_NoNoise",DataMCProduction("2017_PhaseII","2016-v3-average","energy-mcbased-fit",false)));
  production = &fProductions.at("2017_PhaseII_fv_162_10_182_NoNoise");
  production->fDataFiles = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NoMatchNoise_3_8_2017/selection/run_[RunNumber]_tree.root";
  production->fPreMCLocation = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NoMatchNoise_3_8_2017/mc/";
  production->fCutMCLocation = "/nfs/slac/g/exo-userdata/users/mjjewell/Analysis/Reproc_NoMatchNoise_3_8_2017/mc/";
  production->fCutMCname = ".cut.root";



}

bool EXOSourceAgreementManager1D::ReadAvailableProductions()
{
  DataMCProduction *production = 0;

  std::ifstream prodFile;
  prodFile.open(fProductionFilename.Data());

  // Check that production list file has been opened safely
  if(not prodFile.is_open())
  {
    std::cout << "WARNING: Production list file could not be opened." << std::endl;
    std::cout << "File Name:" << fProductionFilename.Data() << std::endl;
    return false;
  }

  std::cout << "Reading production list from file." << std::endl;
  std::map<TString,TString> params;
  std::string line;
  while(std::getline(prodFile,line))
  {
    if(line == "EndOfDataMCProduction : ------------------------------------------------------")
    {
      // Check that all necessary properties are in map
      if(params.count("ProductionName") && params.count("CalibrationTable") && params.count("CalibrationFlavor") && params.count("WeeklyCalibration") && params.count("CutDataLocation") && params.count("PreprocessedMCLocation") && params.count("CutMCLocation") && params.count("CutMCFilenameExtension"))
      {
        // Get values from map (don't actually have to do this but it increases readability)
        TString name = params.at("ProductionName");
        TString title = ((params.count("ProductionTitle"))?(params.at("ProductionTitle")):(name)); // If title not specified just use name
        TString table = params.at("CalibrationTable");
        TString flavor = params.at("CalibrationFlavor");
        TString weekly = params.at("WeeklyCalibration");
        TString dataloc = params.at("CutDataLocation");
        TString premcloc = params.at("PreprocessedMCLocation");
        TString cutmcloc = params.at("CutMCLocation");
        TString ext = params.at("CutMCFilenameExtension");

        weekly.ToLower(); // to avoid case sensitivity

        // Add production to list and set all parameters
        fProductions.insert(std::make_pair(name,DataMCProduction(title,flavor,table,weekly=="true")));
        production = &fProductions.at(name);
        production->fDataFiles = dataloc;
        production->fPreMCLocation = premcloc;
        production->fCutMCLocation = cutmcloc;
        production->fCutMCname = ext;
      }
      else
      {
        // Print warning message
        std::cout << "WARNING: Missing properties in production info file. Production not added to list." << std::endl;
      }
      
      params.clear(); // Clear map and continue reading
    }
    else
    {
      // Make a TString version of line for easy tokenization
      TString tline = line;

      // Delimit by '=' and add property name and value to params map
      TString property = "";
      TString value = "";
      Ssiz_t from = 0;
      tline.Tokenize(property,from,"=");
      tline.Tokenize(value,from,"\n");
      params.insert(std::make_pair(property, value));
    }
  }

  return true;
}
