#include "EXORotationAngleFitter.hh"

ClassImp(EXORotationAngleFitter)

ROOT::Math::IMultiGenFunction* EXORotationAngleFitter::fFCN;

EXORotationAngleFitter::EXORotationAngleFitter(const std::string week, const std::string mult, const double ene, const EXOSourceRunsPolishedInfo* sourceRunsInfo)
{
  fEnergyMCBasedFitter = 0;
  fWeekRunsInfo = 0;
  fFitter = 0;
  fAvgAgree = -1;
  fBestResol = std::numeric_limits<double>::max();

  SetVerboseLevel(0);
  SetSourceRunsInfo(sourceRunsInfo);
  SetWeekIndex(week);
  SetMultiplicity(mult);
  SetMinimizationEnergy(ene);
  SetDenoisedLight(true);
  SaveAllAnglesResults(false);
  SetFitPars(true);
  //SetCalibrationChannel("Rotated");
  fChargeCalib = 0;
  fLightCalib = 0;
  fInitialThPeak = 2615.0;
  SetCalibEnergyBins(5,(fMultiplicityInt <= 1) ? 700 : 0, 4000);

  fResolVsAngle1 = 0; fResolVsAngle2 = 0; fResolVsAngle3 = 0; fResolVsAngle4 = 0;
  
  SetOutputFileName(Form("DefaultNameAngleFitter_Week%s_%s.root",fWeekIndex.c_str(),fMultiplicity.c_str()));

}

EXORotationAngleFitter::~EXORotationAngleFitter()
{
  if(fFitter) delete fFitter;
  if(fEnergyMCBasedFitter) delete fEnergyMCBasedFitter;
  if(fWeekRunsInfo) delete fWeekRunsInfo;
  if(fResolVsAngle1) delete fResolVsAngle1;
  if(fResolVsAngle2) delete fResolVsAngle2;
  if(fResolVsAngle3) delete fResolVsAngle3;
  if(fResolVsAngle4) delete fResolVsAngle4;
}

void EXORotationAngleFitter::Print(bool printRuns)
{
  std::cout << "Fitting rotation angle for:\n";
  std::cout << "Week = " << fWeekIndex << " or " << fWeekIndexInt << std::endl;
  std::cout << "Multiplicity = " << fMultiplicity << " or " << fMultiplicityInt << std::endl;
  std::cout << "Minimization energy = " << fMinimizationEnergyValue << " keV" << std::endl;
  if(printRuns)
  {
    std::cout << "Source runs info:\n";
    fWeekRunsInfo->Print();
  }
}

void EXORotationAngleFitter::SetCalibEnergyBins(int binSize, double lowEne, double upEne)
{
  fCalibBinSize = binSize;
  fCalibLowEnergyFit = lowEne;
  fCalibUpEnergyFit = upEne;
}

void EXORotationAngleFitter::Fcn(int &, double *, double &f, double *x, int /* iflag */)
{
  f = fFCN->operator()(x);
}

double EXORotationAngleFitter::FitFunction(const double* x)
{
  if(fVerboseLevel > 0 && x){
    std::cout << "Using angle: " << x[0] << std::endl;
  }

  if(!RunEnergyFitterWithAngle(x[0]))
    return std::numeric_limits<double>::infinity();

  double resol = fEnergyMCBasedFitter->GetFunction("resol")->Eval(fMinimizationEnergyValue);
  
  if(fVerboseLevel > 0 && x){
    std::cout << "Energy resolution at " << fMinimizationEnergyValue << " is " << resol << " = " << resol*100./fMinimizationEnergyValue << "%" << std::endl;
  }
  
  return resol;
}

bool EXORotationAngleFitter::RunEnergyFitterWithAngle(double angle, bool allCalcs, std::string option)
{
    if (fUseGaussErrfc) {
        if (!AddData(angle))
            return false;
        if (!SetFitFunctions())
            return false;
        if (!FitHistoGaussErrfc())
            return false;
        
        double resol = fFitFunctionGaussErrfc->GetParameter(2)/fFitFunctionGaussErrfc->GetParameter(1);
        //double resol = FitRooHisto();
        
        std::cout << "[GaussErrfc fit] Saving results" << std::endl;
        std::cout << "[GaussErrfc fit] Resolution for angle " << angle << ": " << resol << std::endl;
        
        if (fBestResol > resol) {
            std::cout << "[GaussErrfc fit] Best resolution at angle " << angle << ": " << resol << std::endl;
            
            fBestResol = resol;
            fBestAngle = angle;
        }
        
        if (fSaveAllAnglesResults) {
            option == "bestFit" ? SaveFitResults(angle, resol, true) : SaveFitResults(angle, resol);
        }
        
        return true;
    }
    
  if(fEnergyMCBasedFitter)
    delete fEnergyMCBasedFitter;

  fEnergyMCBasedFitter = new EXOEnergyMCBasedFit1D();
  fEnergyMCBasedFitter->SetVerboseLevel(fVerboseLevel-2);

  if(!AddMC())
    return false;
  if(!AddData(angle))
    return false;
  if(!SetHisto(angle))
    return false;
  if(!SetFitFunctions())
    return false;
  
  fEnergyMCBasedFitter->BinMCEnergy(1);
  fEnergyMCBasedFitter->SetAllFitCalculations(allCalcs);

  std::cout << "Initial parameters calib: p0 = " << fCalibPars[0] << " p1 = " << fCalibPars[1] << " resol: p0 = " << fResolPars[0] << " p1 = " << fResolPars[1] << std::endl;

  if(!fEnergyMCBasedFitter->ExecuteFit(1,"MINIMIZE",1.))
    return false;
    
  double chi2 = fEnergyMCBasedFitter->GetChi2();
  std::cout << "Chi2 = " << chi2 << std::endl;

  double resol = fEnergyMCBasedFitter->GetFunction("resol")->Eval(fMinimizationEnergyValue);

  double chi2_threshold = 5.0;
  if (fMultiplicity == "ms") {chi2_threshold = 8.0;}
  if (option == "charge" || option == "light") {chi2_threshold = 40.0;}
  
  resol /= fMinimizationEnergyValue;
  if(fVerboseLevel > 0)
    std::cout << "RESOL " << resol << std::endl;
  if(resol < 0.01 || resol > 0.5 || chi2 > chi2_threshold || chi2 < 0.0)
  {
    fEnergyMCBasedFitter->SetVerboseLevel(1);
    double dec = 0.0;
    double initialParameter = fCalibPars[1];
    double initialResolParameter = fResolPars[0];
    int attempt = 0;
    do
    {
      if(fVerboseLevel > 0)
        std::cout << "Trying other initial calib parameters before giving up: " << fCalibPars[1] << "\n";
      //SetFitPars(true);
      fCalibPars[1] = 1.2*initialParameter - dec;

      fEnergyMCBasedFitter->GetFunction("calib")->SetParameters(fCalibPars[0],fCalibPars[1]);
      fEnergyMCBasedFitter->GetFunction("calib")->SetParError(0,0.1*std::fabs(fCalibPars[0]));
      fEnergyMCBasedFitter->GetFunction("calib")->SetParError(1,0.1*std::fabs(fCalibPars[1]));
      
      fEnergyMCBasedFitter->GetFunction("resol")->SetParameters(fResolPars[0],fResolPars[1]);
      fEnergyMCBasedFitter->GetFunction("resol")->SetParError(0,std::fabs(fResolPars[0]));
      fEnergyMCBasedFitter->GetFunction("resol")->SetParError(1,std::fabs(fResolPars[1]));
      
      if(fEnergyMCBasedFitter->GetFunction("trigeff"))
      {
      fEnergyMCBasedFitter->GetFunction("trigeff")->SetParameters(fTrigEffPars[0],fTrigEffPars[1]);
      fEnergyMCBasedFitter->GetFunction("trigeff")->SetParError(0,std::fabs(fTrigEffPars[0]));
      fEnergyMCBasedFitter->GetFunction("trigeff")->SetParError(1,std::fabs(fTrigEffPars[1]));
      }

      std::cout << "Setting parameters calib: p0 = " << fCalibPars[0] << " p1 = " << fCalibPars[1] << " resol: p0 = " << fResolPars[0] << " p1 = " << fResolPars[1] << std::endl;
      
      //if(!fEnergyMCBasedFitter->ExecuteFit(1,"SIMPLEX",1.))
      //  return false;
      if(!fEnergyMCBasedFitter->ExecuteFit(1,"MINIMIZE",1.))
        return false;
      resol = fEnergyMCBasedFitter->GetFunction("resol")->Eval(fMinimizationEnergyValue);
      resol /= fMinimizationEnergyValue;

      chi2 = fEnergyMCBasedFitter->GetChi2();
      std::cout << "chi2 = " << chi2 << std::endl;
      
      if(fVerboseLevel > 0)
        std::cout << "RESOL " << resol << std::endl;
      
      if(resol > 0.012 && resol < 0.5 && chi2 < chi2_threshold && chi2 > 0.0)
        break;
      
      dec += 0.05;
        
        if (fCalibPars[1] <= 0.55) {
            std::cout << "Trying different resolution parameter" << std::endl;
            fCalibPars[1] = initialParameter;
            dec = 0.0;
            
            switch (attempt) {
                case 0:
                {
                    fResolPars[0] = initialResolParameter - 0.05;
                    attempt = 1;
                }
                    break;
                case 1:
                {
                    fResolPars[0] = initialResolParameter - 0.1;
                    attempt = 2;
                }
                    break;
                case 2:
                {
                    fResolPars[0] = initialResolParameter + 0.05;
                    attempt = 3;
                }
                    break;
                case 3:
                {
                    fResolPars[0] = initialResolParameter + 0.1;
                    attempt = 4;
                }
                    break;
            }
            
            fResolPars[1] = 10.0;
        }
        
        std::cout << "dec = " << dec << " calib = " << fCalibPars[1] << " attempt = " << attempt << std::endl;
    } while (dec < 0.9 && fCalibPars[1] > 0.55 && attempt < 4);
      
    if(dec >= 0.9 || fCalibPars[1] <= 0.55 || attempt >= 4)
    {
      std::cerr << "Problem with one of the fits in scan. Angle = " << angle << ". Exiting..." << std::endl;
      //exit(1);

      fCalibPars[1] = initialParameter;
        
      return false;
    }
    fEnergyMCBasedFitter->SetVerboseLevel(0);
  }
  
  if(option == "charge")
  {
    fChargeCalib = new TF1(*fEnergyMCBasedFitter->GetFunction("calib"));
    if(fSaveAllAnglesResults)
    {
      TString tempName = fOutputFileName;
      tempName.ReplaceAll(".root","_AngleZero.root");
      fEnergyMCBasedFitter->SaveHistosIn(tempName.Data(),"RECREATE");
    }
    fAvgAgree += fEnergyMCBasedFitter->GetMinuit()->fAmin;
    fAvgAgree /= 2.;
    if(fVerboseLevel > 0)
      std::cout << "Channel agree: " << fEnergyMCBasedFitter->GetMinuit()->fAmin << " average = " << fAvgAgree << std::endl;
    return true;
  }
  if(option == "light")
  {
    fLightCalib = new TF1(*fEnergyMCBasedFitter->GetFunction("calib"));
    if(fSaveAllAnglesResults)
    {
      TString tempName = fOutputFileName;
      tempName.ReplaceAll(".root","_AnglePi2.root");
      fEnergyMCBasedFitter->SaveHistosIn(tempName.Data(),"RECREATE");
    }
    fAvgAgree = fEnergyMCBasedFitter->GetMinuit()->fAmin;
    if(fVerboseLevel > 0)
      std::cout << "Channel agree: " << fAvgAgree << std::endl;
    return true;
  }
  
  /*
  double min = fEnergyMCBasedFitter->GetMinuit()->fAmin;

  if(fAvgAgree < 0)
    fAvgAgree = min;
  else if(min < fAvgAgree*10)
  {
    fAvgAgree += min;
    fAvgAgree /= 2.;
  }
  else
  {
    SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0],fBestTrigEffPars[0],fBestTrigEffPars[1]);
    return false;
  }
  */
    
  fCalibPars[0] = fEnergyMCBasedFitter->GetFunction("calib")->GetParameter(0);
  fCalibPars[1] = fEnergyMCBasedFitter->GetFunction("calib")->GetParameter(1);
  fResolPars[0] = std::abs(fEnergyMCBasedFitter->GetFunction("resol")->GetParameter(0));
  fResolPars[1] = std::abs(fEnergyMCBasedFitter->GetFunction("resol")->GetParameter(1));
  if(fEnergyMCBasedFitter->GetFunction("trigeff"))
  {
    fTrigEffPars[0] = fEnergyMCBasedFitter->GetFunction("trigeff")->GetParameter(0);
    fTrigEffPars[1] = fEnergyMCBasedFitter->GetFunction("trigeff")->GetParameter(1);
  }
  
  resol = fEnergyMCBasedFitter->GetFunction("resol")->Eval(fMinimizationEnergyValue);
  
  std::cout << "Final parameters calib: p0 = " << fCalibPars[0] << " p1 = " << fCalibPars[1] << " resol: p0 = " << fResolPars[0] << " p1 = " << fResolPars[1] << std::endl;
  std::cout << "fAmin = " << fEnergyMCBasedFitter->GetMinuit()->fAmin << std::endl;

  if(fBestResol > resol)
  {
    if(fVerboseLevel > 0)
    {
      std::cout << "Best resol pars for angle " << angle << " with resolution " << resol << " at " << fMinimizationEnergyValue << std::endl;
      std::cout << "Setting " << fCalibPars[0] << " and " << fCalibPars[1] << std::endl;
    }
    
    fBestResol = resol;
    fBestAngle = angle;
    fBestCalibPars[0] = fCalibPars[0];     fBestCalibPars[1] = fCalibPars[1];
    fBestResolPars[0] = fResolPars[0];     fBestResolPars[1] = fResolPars[1];
    if(fEnergyMCBasedFitter->GetFunction("trigeff"))
    {
      fBestTrigEffPars[0] = fTrigEffPars[0]; fBestTrigEffPars[1] = fTrigEffPars[1];
    }
  }
  
  if(fSaveAllAnglesResults)
  {
    TString tempName = fOutputFileName;
    tempName.ReplaceAll(".root",Form("_Angle%.5f_Resol%.5f.root",angle,resol));
    fEnergyMCBasedFitter->SaveHistosIn(tempName.Data(),"RECREATE");
  }

  return true;
}

bool EXORotationAngleFitter::FitHistoGaussErrfc()
{
    std::cout << "[GaussErrfc fit] Fitting" << std::endl;
    
    if (!fFitHisto) {
        std::cout << "[GaussErrfc fit] no data in histogram" << std::endl;
        
        return false;
    }
    
    if (!fFitFunctionGaussErrfc) {
        std::cout << "[GaussErrfc fit] fit function not set" << std::endl;
        
        return false;
    }
    
    TFitResultPtr r = fFitHisto->Fit(fFitFunctionGaussErrfc,"SRN");
    
    //r->Print("V");
    
    fFitHisto->Draw();
    fFitFunctionGaussErrfc->Draw("same");
    
    return true;
}

double EXORotationAngleFitter::FitRooHisto()
{
    std::cout << "[RooFit] Fitting" << std::endl;
    
    if (!fFitHisto) {
        std::cout << "[RooFit] no data in histogram" << std::endl;
        
        return false;
    }
    
    RooRealVar fRotatedEnergy("RotatedEnergy","RotatedEnergy",0,fUpEnergyFit);
    
    RooDataHist fRooDataHist("DataSet","DataSet",RooArgList(fRotatedEnergy),fFitHisto,1.0);
    
    RooRealVar fFitMean("Mean","Mean",fInitialThPeak,fLowEnergyFit,fUpEnergyFit);
    RooRealVar fFitSigma("Sigma","Sigma",50,0,0.3*fInitialThPeak);
    RooRealVar fErrfcFrac("ErrfcFrac","ErrfcFrac",0.6,0,1);
    
    RooGaussian fRooGaussian("Gauss","Gauss",fRotatedEnergy,fFitMean,fFitSigma);
    RooGenericPdf fErrfc("Errfc","Errfc","0.5*TMath::Erfc((@0-@1)/(TMath::Sqrt(2)*@2))",RooArgList(fRotatedEnergy,fFitMean,fFitSigma));
    RooAddPdf fRooFitFunction("RooFitFunction","RooFitFunction",fRooGaussian,fErrfc,fErrfcFrac);
    
    fRooFitFunction.fitTo(fRooDataHist,RooFit::Range(fLowEnergyFit,fUpEnergyFit));
    
    std::cout << "[RooFit] Plotting data" << std::endl;
    
    /*if (fPlotFrame)
        delete fPlotFrame;*/
    fPlotFrame = fRotatedEnergy.frame(RooFit::Bins(200),RooFit::Range(fLowEnergyFit,fUpEnergyFit));
    
    fRooDataHist.plotOn(fPlotFrame,RooFit::DrawOption("ZP"),RooFit::MarkerStyle(20),RooFit::MarkerSize(0.5));
    fRooFitFunction.plotOn(fPlotFrame);
    fRooFitFunction.plotOn(fPlotFrame,RooFit::Components("Gauss"),RooFit::LineWidth(2),RooFit::LineStyle(2));
    fRooFitFunction.plotOn(fPlotFrame,RooFit::Components("Errfc"),RooFit::LineWidth(2),RooFit::LineStyle(2));
    
    return fFitSigma.getVal()/fFitMean.getVal();
}

TH1F *EXORotationAngleFitter::GetFitFunctionHisto()
{
    if (!fFitFunctionGaussErrfc)
        return 0;
    
    TH1F *h = new TH1F("h","h",fNbinsFit,fLowEnergyFit,fUpEnergyFit);
    
    for (int i = 1; i < fNbinsFit; i++) {
        double energy = h->GetXaxis()->GetBinCenter(i);
        double v = fFitFunctionGaussErrfc->Eval(energy);
        
        h->SetBinContent(i,v);
    }
    
    return h;
}

bool EXORotationAngleFitter::SaveFitResults(double angle, double resol, bool bestFit)
{
    TH1F *histoFit = GetFitFunctionHisto();
    histoFit->SetLineColor(kBlue);
    
    fFitHisto->SetTitle(Form("Data angle = %.5f, resolution = %.5f",angle,resol));
    
    TString tempName = fOutputFileName;
    if (bestFit) {tempName.ReplaceAll(".root","_BestResolFit.root");}
    else {tempName.ReplaceAll(".root",Form("_Angle%.5f_Resol%.5f.root",angle,resol));}
    
    TFile outFile(tempName,"RECREATE");
    outFile.cd();
    fFitHisto->Write("FitHisto_data");
    histoFit->Write("FitHisto_func");
    outFile.Close();
    
    fFitHisto->SetMarkerStyle(20);
    fFitHisto->SetMarkerSize(0.5);
    
    histoFit->SetLineWidth(2);
    
    tempName.ReplaceAll(".root",".pdf");
    TCanvas *c1 = new TCanvas();
    c1->cd();
    fFitHisto->Draw("ZP");
    histoFit->Draw("same");
    //fPlotFrame->Draw();
    c1->Print(tempName);
    
    return true;
}

bool EXORotationAngleFitter::AddMC()
{
  for(std::map<std::string, std::vector<double> >::iterator mc = fEnergyMC.begin(); mc != fEnergyMC.end(); mc++)
  {
    TString mcId(mc->first.c_str());
    //std::cout << "Adding MC: " << mcId.Data() << std::endl;

    if(!fEnergyMCBasedFitter->AddMC(mcId.Data(),0,0,0,0,0,&fEnergyMC.at(mc->first)[0],&fEnergyWeight.at(mc->first)[0],mc->second.size()))
      return false;
  }

  return true;
}

bool EXORotationAngleFitter::AddData(double angle)
{
  const std::vector<std::string> inputFileNames = fWeekRunsInfo->GetListOf("DataSelectionFileName");
  const std::vector<std::string> mcIds = fWeekRunsInfo->GetListOf("MCId");
  const std::vector<std::string> mcMultWildCards = fWeekRunsInfo->GetListOf("MCMultiplicityWildCard");
  const std::vector<std::string> dataIds = fWeekRunsInfo->GetListOf("DataId");
  const std::vector<std::string> dataMultWildCards = fWeekRunsInfo->GetListOf("DataMultiplicityWildCard");
  const std::vector<std::string> weeks = fWeekRunsInfo->GetListOf("WeekIndex");
  const std::vector<std::string> prescales = fWeekRunsInfo->GetListOf("TriggerPrescale"); 

  if(inputFileNames.empty())
  {
    std::cerr << "Cannot AddData, incompatible number files!\n";
    return false;
  }

  const std::vector<std::string> sourceZs = fWeekRunsInfo->GetListOf("SourceZ");
  const std::vector<std::string> sourceAs = fWeekRunsInfo->GetListOf("SourceA");
  const std::vector<std::string> sourcePosXs = fWeekRunsInfo->GetListOf("SourcePositionX");
  const std::vector<std::string> sourcePosYs = fWeekRunsInfo->GetListOf("SourcePositionY");
  const std::vector<std::string> sourcePosZs = fWeekRunsInfo->GetListOf("SourcePositionZ");  
  
  bool allOk = true;
  
    if (fUseGaussErrfc) {
        std::cout << "[GaussErrfc fit] Setting histo range" << std::endl;
        AdjustHistoRanges(angle);
        
        std::cout << "[GaussErrfc fit] Setting histogram" << std::endl;
    }
  
  /*if (fFitHisto)
    delete fFitHisto;*/
  fFitHisto = new TH1F("FitHisto","FitHisto",fNbinsFit,fLowEnergyFit,fUpEnergyFit);
  
  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {   
    TString mcId = mcIds.at(i);
    mcId = mcId.ReplaceAll(mcMultWildCards.at(i).c_str(),fMultiplicity.c_str());

    TString dataId = dataIds.at(i);
    dataId = dataId.ReplaceAll(dataMultWildCards.at(i).c_str(),fMultiplicity.c_str());

    size_t nEnergies = fEnergyCharge.at(dataId.Data()).size();
    std::vector<double> energy(nEnergies);

    for(size_t j = 0; j < nEnergies; j++) {
      energy[j] = cos(angle)*fEnergyCharge.at(dataId.Data()).at(j) + sin(angle)*fEnergyScint.at(dataId.Data()).at(j);
      
      fFitHisto->Fill(energy[j]);
    }
    
    //if(fVerboseLevel > 0)
    //  std::cout << "Adding data: " << dataId.Data() << " associated to " << mcId.Data() << std::endl;
    
    if (!fUseGaussErrfc) {
    if(!fEnergyMCBasedFitter->AddData(dataId.Data(),mcId.Data(),atoi(sourceZs.at(i).c_str()),atoi(sourceAs.at(i).c_str()),atof(sourcePosXs.at(i).c_str()),atof(sourcePosYs.at(i).c_str()),atof(sourcePosZs.at(i).c_str()),atoi(prescales.at(i).c_str()),&energy[0],(int)energy.size()))//&e_rotated[0],(int)e_rotated.size()))
      allOk = false;
    }
  }

  return allOk;  
}

void EXORotationAngleFitter::AdjustHistoRanges(double angle, bool incCs137, bool hasWeek5or6or7)
{
  //fLowEnergyFit = (fMultiplicityInt == 1) ? 1000 : 0;
  //fUpEnergyFit = (fMultiplicityInt == 1) ? 5000 : 5000;
  if(!fChargeCalib || !fLightCalib)
  {
    fLowEnergyFit = 1600;
    fUpEnergyFit = (fDenoisedLight) ? 4000 : 10000;
  }
  else
  {
    fLowEnergyFit = fChargeCalib->GetX(fCalibLowEnergyFit)*cos(angle) + fLightCalib->GetX(fCalibLowEnergyFit)*sin(angle) ;
    fUpEnergyFit = fChargeCalib->GetX(fCalibUpEnergyFit)*cos(angle) + fLightCalib->GetX(fCalibUpEnergyFit)*sin(angle) ;
    fInitialThPeak = fChargeCalib->GetX(2615.0)*cos(angle) + fLightCalib->GetX(2615.0)*sin(angle);
  }

  if(incCs137)
  {
    //if(fLowEnergyFit > 600)
      //fLowEnergyFit = fChargeCalib->GetX(600)*cos(angle) + fLightCalib->GetX(600)*sin(angle);
    //fLowEnergyFit = 800.;
  }

  if(hasWeek5or6or7 && fMultiplicityInt == 2)
  {
    fLowEnergyFit = fChargeCalib->GetX(1000)*cos(angle) + fLightCalib->GetX(1000)*sin(angle) ;
    fUpEnergyFit = fChargeCalib->GetX(2000)*cos(angle) + fLightCalib->GetX(2000)*sin(angle) ;
    //fLowEnergyFit = 1300.;
    //fUpEnergyFit = 3000.;
  }
  
  fNbinsFit = static_cast<int>(std::ceil((fUpEnergyFit-fLowEnergyFit)/fCalibBinSize));
  
  return;
}

bool EXORotationAngleFitter::SetHisto(double angle)
{
  const std::vector<std::string> dataIds = fWeekRunsInfo->GetListOf("DataId");
  const std::vector<std::string> dataMultWildCards = fWeekRunsInfo->GetListOf("DataMultiplicityWildCard");
  const std::vector<std::string> sourceNames = fWeekRunsInfo->GetListOf("SourceName");

  bool hasCs137 = false;
  std::vector<TString> fitHistos;
  size_t n = dataIds.size();
  for(size_t i = 0; i < n; i++)
  {
    TString dataId = dataIds.at(i);
    dataId = dataId.ReplaceAll(dataMultWildCards.at(i).c_str(),fMultiplicity.c_str());
    fitHistos.push_back(dataId.Data());
    if(sourceNames.at(i) == "Cs-137")
      hasCs137 = true;
  }

  bool hasWeek5or6or7 = false;
  const std::vector<std::string> weeks = fWeekRunsInfo->GetListOf("WeekIndex");
  if(std::find(weeks.begin(),weeks.end(),"005") != weeks.end()) hasWeek5or6or7 = true;
  else if(std::find(weeks.begin(),weeks.end(),"006") != weeks.end()) hasWeek5or6or7 = true;
  else if(std::find(weeks.begin(),weeks.end(),"007") != weeks.end()) hasWeek5or6or7 = true;
  
  AdjustHistoRanges(angle, hasCs137, hasWeek5or6or7);
  
  fEnergyMCBasedFitter->SetDataHisto("FitAllHisto","",fitHistos,fNbinsFit,fLowEnergyFit,fUpEnergyFit,1);

  return true;
}

bool EXORotationAngleFitter::SetFitFunctions()
{
    if (fUseGaussErrfc) {
        /*if (fFitFunctionGaussErrfc)
            delete fFitFunctionGaussErrfc;*/
        
        std::cout << "[GaussErrc fit] Setting fit function" << std::endl;
        
        fFitFunctionGaussErrfc = new TF1("FitFunction","[0]*TMath::Gaus(x,[1],[2])+[3]*0.5*TMath::Erfc((x-[1])/(TMath::Sqrt(2)*[2]))",fLowEnergyFit,fUpEnergyFit);
        
        fFitFunctionGaussErrfc->SetParameters(500,fInitialThPeak,80,50);
        
        return true;
    }

  TF1 *calibFunc = new TF1("calib","[0]+[1]*x",0,10000);
  //double calibPars[] = {10,0.75};
  calibFunc->SetParameters(fCalibPars);
  double errors[] = {0.1*std::fabs(fCalibPars[0]),0.1*std::fabs(fCalibPars[1])};
  calibFunc->SetParErrors(errors);
  calibFunc->SetParLimits(0,-200,200);
  calibFunc->SetParLimits(1,1e-7,2.0);
  //calibFunc->SetParLimits(0,0.01*fCalibPars[0],100.0*fCalibPars[0]);
  //calibFunc->SetParLimits(1,0.1*fCalibPars[1],2.0*fCalibPars[1]);
  fEnergyMCBasedFitter->SetFunction("calib",calibFunc);
    
  TF1 *resolFunc = new TF1("resol","sqrt([0]*[0]*x + [1]*[1])",0,10000);
  //double resolPars[] = {0.1,20.};//double resolPars[] = {0.1,20.};
  resolFunc->SetParameters(fResolPars);
  resolFunc->SetParErrors(fResolPars);
  resolFunc->SetParLimits(0,1e-7,10);
  resolFunc->SetParLimits(1,-200,200);
  //resolFunc->SetParLimits(0,0.1*fResolPars[0],3.0*fResolPars[0]);
  //resolFunc->SetParLimits(1,0.10*fResolPars[1],100.0*fResolPars[1]);
  fEnergyMCBasedFitter->SetFunction("resol",resolFunc);

  
  if(fEnergyMCBasedFitter->ShouldSetTriggerEffFunction())
  {
    TF1 *trigEffFunc = new TF1("trigeff","0.5*(1+TMath::Erf(0.7071067811865474*(x-[0])/[1]))",0,10000);     
    trigEffFunc->SetParameters(fTrigEffPars);
    trigEffFunc->SetParErrors(fTrigEffPars);
    //trigEffFunc->SetParLimits(0,1,5000);
    //trigEffFunc->SetParLimits(1,1,500);
    fEnergyMCBasedFitter->SetFunction("trigeff",trigEffFunc);
    //isSet = true;
  }
  
  return true;  
}

void EXORotationAngleFitter::SetFitPars(bool firstFit, double calibP0, double calibP1, double resolP0, double resolP1, double trigP0, double trigP1)
{
  if(firstFit)
  {
    if(fVerboseLevel > 0)
      std::cout << "Set to use initial calib pars..." << std::endl;

    fCalibPars[0] = 20.;     fCalibPars[1] = 1.0;
    fResolPars[0] = 0.9;     fResolPars[1] = 50.;
    fTrigEffPars[0] = 1250.; fTrigEffPars[1] = 150.;

    // start with some better values for weeks where MC-based fit was observed to fail during scan
    /*
    if(fWeekIndex == "003")
    {
      fCalibPars[0] = 20.;     fCalibPars[1] = 0.75;
      fResolPars[0] = 1.;     fResolPars[1] = 0.001;
    }
    else if(fWeekIndex == "059")
    {
      fResolPars[0] = 0.6;     fResolPars[1] = 20.5;
    }
    else if(fWeekIndex == "111")
    {
      fCalibPars[0] = 5.;     fCalibPars[1] = 0.75;
      fResolPars[0] = 0.8;     fResolPars[1] = 15;
    }
    */
  }
  else
  {
    if(fVerboseLevel > 0)
      std::cout << "Set to use calib pars " << calibP0 << " and " << calibP1 << std::endl;
  
    fCalibPars[0] = calibP0;  fCalibPars[1] = calibP1;
    fResolPars[0] = resolP0;  fResolPars[1] = resolP1;
    fTrigEffPars[0] = trigP0; fTrigEffPars[1] = trigP1;
  }
}

void EXORotationAngleFitter::SetWeekIndex(const std::string week)
{
  if(fVerboseLevel > 0)
    std::cout << "Setting week to " << week << std::endl;
  
  int weekInt = atoi(week.c_str());
  if(weekInt < 0 || weekInt > 1000)
  {
    std::cerr << "Invalid week, try 061 for example!\n";
    return;
  }
  fWeekIndexInt = weekInt;

  fWeekIndex = week;
  if(fWeekRunsInfo)
    delete fWeekRunsInfo;
  fWeekRunsInfo = new EXOSourceRunsPolishedInfo(*fSourceRunsInfo);
  fWeekRunsInfo->CutExact("WeekIndex",fWeekIndex);
}

void EXORotationAngleFitter::SetMultiplicity(std::string mult)
{
  std::transform(mult.begin(),mult.end(),mult.begin(), ::tolower);
  if(fVerboseLevel > 0)
    std::cout << "Setting multiplicity to " << mult << std::endl;
  
  fMultiplicity = mult;
  fMultiplicityInt = (mult == "ss") ? 1 : 2;
}

void EXORotationAngleFitter::SetMinimizationEnergy(double energy)
{
  fMinimizationEnergyValue = energy;
}

bool EXORotationAngleFitter::FitChannelsCalibPars()
{
  bool allOk = true;
  if(fVerboseLevel > 0)
    std::cout << "Fitting light energy (angle = pi/2)...\n";
  double tempCalib = fCalibPars[1];
  double tempResol = fResolPars[1];
  if(!fDenoisedLight)
  {
    fCalibPars[1] = fCalibPars[1]/3.;
    fResolPars[1] = 2*fResolPars[1];
  }
    fCalibPars[0] = -150;
    fCalibPars[1] = 0.4;
    fResolPars[0] = 3.0;
    fResolPars[1] = 20;
  if (fDenoisedLight) {
    fCalibPars[0] = 10.0;
    fCalibPars[1] = 1.0;
    fResolPars[0] = 0.1;
    fResolPars[1] = 20;
  }
  if(!RunEnergyFitterWithAngle(1.570796,true,"light"))
    allOk = false;
  fCalibPars[1] = tempCalib;
  fResolPars[1] = tempResol;
  if(fVerboseLevel > 0)
    fLightCalib->Print();

  if(fVerboseLevel > 0)
    std::cout << "Fitting charge energy (angle = 0)...\n";
  tempCalib = fCalibPars[1];
  tempResol = fResolPars[1];
    fCalibPars[0] = 50;
    fCalibPars[1] = 1.0;
    fResolPars[0] = 0.3;
    fResolPars[1] = 20;
  if(!RunEnergyFitterWithAngle(0.,true,"charge"))
    allOk = false;
  fCalibPars[1] = tempCalib;
  fResolPars[1] = tempResol;
  if(fVerboseLevel > 0)
    fChargeCalib->Print();

  return allOk;
}

bool EXORotationAngleFitter::RunScanAndFit()
{
  if(fVerboseLevel > 0)
    std::cout << "Scanning and fitting rotation angle using the energy MC-based fit...\n";
  
  if(!PrepareEnergyVectors())
  {
    std::cerr << "Cannot execute fit, failed to fill energy vectors. Check if there is any valid runs or events in this week = " << fWeekIndex << std::endl;
    return false;
  }

  bool useGaussErrfcTmp = fUseGaussErrfc;
  fUseGaussErrfc = false;
  if(!FitChannelsCalibPars())
  {
    std::cerr << "Not able to determine preliminary channels calibration. Quitting...\n";
    return false;
  }
  fUseGaussErrfc = useGaussErrfcTmp;

    Scanner(0.,1.0,0.1,fResolVsAngle1,3,true);
    SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0],fBestTrigEffPars[0],fBestTrigEffPars[1]);
    Scanner(fBestAngle-0.15,fBestAngle+0.11,0.05,fResolVsAngle2);
    SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0],fBestTrigEffPars[0],fBestTrigEffPars[1]);
    Scanner(fBestAngle-0.06,fBestAngle+0.051,0.01,fResolVsAngle3);
    //SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0],fBestTrigEffPars[0],fBestTrigEffPars[1]);
    //Scanner(fBestAngle-0.015,fBestAngle+0.011,0.004,fResolVsAngle4);
  
  return true;
}

bool EXORotationAngleFitter::RunFitForAngle(double angle)
{
    std::cout << "Running fit for angle " << angle << std::endl;
    
    if(!PrepareEnergyVectors())
    {
        std::cerr << "Cannot execute fit, failed to fill energy vectors. Check if there is any valid runs or events in this week = " << fWeekIndex << std::endl;
        return false;
    }
    
    fChargeCalib = new TF1("calib","[0]+[1]*x",0,10000);
    fLightCalib = new TF1("calib","[0]+[1]*x",0,10000);
    
    fChargeCalib->SetParameters(75.34,0.98);
    fLightCalib->SetParameters(-99.36,1.044);
    
    if (!RunEnergyFitterWithAngle(angle,true)) {return false;}
    
    return true;
}
  
bool EXORotationAngleFitter::Scanner(double start, double stop, double step, TGraphErrors*& graph, int early, bool setInitialGuess)
{
  if(graph)
    delete graph;
  graph = new TGraphErrors();

  double prevResol = std::numeric_limits<double>::max();
  int tempEarly = 0;
  for(double angle = start; angle < stop; angle += step)
  {    
    if(fVerboseLevel > 0){
      std::cout << "Using angle: " << angle << std::endl;
    }
      
      if (setInitialGuess && !fUseGaussErrfc) {
          if (angle >= 0.0 && angle < 0.05) {fCalibPars[1] = 1.0; fResolPars[0] = 2.0;}
          if (angle >= 0.05 && angle < 0.15) {fCalibPars[1] = 1.0; fResolPars[0] = 1.8;}
          if (angle >= 0.15 && angle < 0.25) {fCalibPars[1] = 0.9; fResolPars[0] = 1.5;}
          if (angle >= 0.25 && angle < 0.35) {fCalibPars[1] = 0.85; fResolPars[0] = 1.1;}
          if (angle >= 0.35 && angle < 0.45) {fCalibPars[1] = 0.8; fResolPars[0] = 0.9;}
          if (angle >= 0.45 && angle < 0.55) {fCalibPars[1] = 0.75; fResolPars[0] = 0.7;}
          if (angle >= 0.55 && angle < 0.65) {fCalibPars[1] = 0.75; fResolPars[0] = 0.8;}
          if (angle >= 0.65 && angle < 0.75) {fCalibPars[1] = 0.75; fResolPars[0] = 0.8;}
          if (angle >= 0.75 && angle < 0.85) {fCalibPars[1] = 0.75; fResolPars[0] = 0.9;}
          if (angle >= 0.85 && angle < 0.95) {fCalibPars[1] = 0.75; fResolPars[0] = 1.0;}
          if (angle >= 0.95 && angle < 1.05) {fCalibPars[1] = 0.8; fResolPars[0] = 1.2;}
          
          if (angle < 0.6) {fCalibPars[0] = 10.0;}
          else {fCalibPars[0] = -15.0;}
          fResolPars[1] = 10.0;
      }
      
    if(!RunEnergyFitterWithAngle(angle,true))
      continue;

    if (!fUseGaussErrfc) {
        double agree = fEnergyMCBasedFitter->GetMinuit()->fAmin;
        if(agree > 2.5*fAvgAgree)
        {
            if(fVerboseLevel > 0)
            std::cout << "Angle: " << angle << " not added in current pol2 fit, fAmin = " << agree << " while channels agree " << fAvgAgree << std::endl;
            continue;
        }
    }
    
    double resol;
    double error;
    
      if (!fUseGaussErrfc) {
          resol = fEnergyMCBasedFitter->GetPeakWidth(fMinimizationEnergyValue);//GetFunction("resol")->Eval(fMinimizationEnergyValue);
          error = fEnergyMCBasedFitter->GetPeakWidthError(fMinimizationEnergyValue);
          //double p0 =  fEnergyMCBasedFitter->GetFunction("resol")->GetParameter(0);
          //double p1 =  fEnergyMCBasedFitter->GetFunction("resol")->GetParameter(1);
          //double ep0 =  fEnergyMCBasedFitter->GetFunction("resol")->GetParError(0);
          //double ep1 =  fEnergyMCBasedFitter->GetFunction("resol")->GetParError(1);
          //double error = 1/resol * sqrt((fMinimizationEnergyValue*p0*ep0)*(fMinimizationEnergyValue*p0*ep0) + (p1*ep1)*(p1*ep1));
      }
      else {
          resol = fFitFunctionGaussErrfc->GetParameter(2)/fFitFunctionGaussErrfc->GetParameter(1);
          error = TMath::Sqrt((fFitFunctionGaussErrfc->GetParError(2))*(fFitFunctionGaussErrfc->GetParError(2))/fFitFunctionGaussErrfc->GetParameter(1)/fFitFunctionGaussErrfc->GetParameter(1) + (fFitFunctionGaussErrfc->GetParError(1))*(fFitFunctionGaussErrfc->GetParError(1))*(fFitFunctionGaussErrfc->GetParameter(2))*(fFitFunctionGaussErrfc->GetParameter(2))/fFitFunctionGaussErrfc->GetParameter(1)/fFitFunctionGaussErrfc->GetParameter(1)/fFitFunctionGaussErrfc->GetParameter(1)/fFitFunctionGaussErrfc->GetParameter(1));
      }
    
    if(early != 0)
    {
      if(resol > prevResol)
        tempEarly++;
      else
        tempEarly = 0;
    }
    prevResol = resol;
    
    int n = graph->GetN();
    graph->SetPoint(n,angle,resol);
    graph->SetPointError(n,0,error);

    if(fVerboseLevel > 0){
        if (fUseGaussErrfc) {std::cout << "[GaussErrfc fit] Energy resolution at 2615.0 is " << resol << " = " << resol*100.0 << "%" << std::endl;}
        else {std::cout << "Energy resolution at " << fMinimizationEnergyValue << " is " << resol << " = " << resol*100./fMinimizationEnergyValue << "%" << std::endl;}
    }

    if(early != 0 && tempEarly >= early)
      break;
  }
    
    if (graph->GetN() == 0) {std::cout << "No data in pol2 fit" << std::endl; return false;}
  
  graph->Fit("pol2",(fVerboseLevel > 0) ? "W" : "QW");
  fBestAngle = -0.5* graph->GetFunction("pol2")->GetParameter(1)/graph->GetFunction("pol2")->GetParameter(2);
  fBestResol = graph->GetFunction("pol2")->Eval(fBestAngle);
  if(fVerboseLevel > 0) {
      if (fUseGaussErrfc) {std::cout << "[GaussErrfc fit] Current best angle = " << fBestAngle << " with resolution = " << fBestResol*100.0 << "%" << std::endl;}
      else {std::cout << "Current best angle = " << fBestAngle << " with resolution = " <<  fBestResol*100./fMinimizationEnergyValue << "%" << std::endl;}
  }

  return true;
}
  
bool EXORotationAngleFitter::ExecuteFit(const char* minimizer, Float_t fitPrecision)
{
  minimizer = "";
  fitPrecision = 0.0;

  if(fVerboseLevel > 0)
    std::cout << "Executing energy MC-based fit...\n";

  if(!PrepareEnergyVectors())
  {
    std::cerr << "Cannot execute fit, failed to fill energy vectors. Check if there is any valid runs or events in this week = " << fWeekIndex << std::endl;
    return false;
  }

  if(!FitChannelsCalibPars())
  {
    std::cerr << "Not able to determine preliminary channels calibration. Quitting...\n";
    return false;
  }
  
  fFitFunction = new ROOT::Math::Functor(this, &EXORotationAngleFitter::FitFunction,1);
  const ROOT::Math::IMultiGenFunction& func = *fFitFunction;
  fFCN = const_cast<ROOT::Math::IMultiGenFunction *>(&func);

  if(fFitter)
    delete fFitter;

  fFitter = new TFitter(1);
  fFitter->GetMinuit()->SetPrintLevel(fVerboseLevel);
  fFitter->SetFCN(&EXORotationAngleFitter::Fcn);

  fFitter->SetParameter(0,"RotationAngle",0.,0.01,0.0000001,1.57); // start at angle = 0, last fit was charge energy only

  double commandList[10];
  int errorFlag;

  errorFlag = 0;

  //fFitter->ExecuteCommand("SHOw FCNvalue",commandList,0);
  //commandList[0] = (fitType == 2) ? 1 : 0.5;
  //fFitter->ExecuteCommand("SET ERR",commandList,1);

  commandList[0] = 0;
  commandList[1] = 16;
  commandList[2] = 0;
  commandList[3] = 1.5;
  fFitter->ExecuteCommand("SCAn",commandList,4);

  double min = fFitter->GetParameter(0);
  if(fVerboseLevel > 0)
    std::cout << "MIN FOUND AFTER 1st SCAN = " << min << std::endl;

  double step[] = {0.05,0.01};//{0.1,0.05};
  double from[] = {0.05,0.04};//{0.4,0.05};
  int nsteps[] = {3,9};
  for(int i = 0; i < 8; i++)
  {
    double start = min - from[i%2];
    commandList[0] = 0;
    commandList[1] = nsteps[i%2];
    commandList[2] = start;//- 0.3;
    commandList[3] = start + (nsteps[i%2]-1)*step[i%2]; //+ 0.3;

    //double corrRatio = (cos(fBestAngle)+sin(fBestAngle))/(cos(start)+sin(start));
    SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0],fBestTrigEffPars[0],fBestTrigEffPars[1]);

    fFitter->ExecuteCommand("SCAn",commandList,4);

    min = fFitter->GetParameter(0);
    if(fVerboseLevel > 0)
      std::cout << "MIN FOUND AFTER " << i+1 << " SCANS = " << min << std::endl;

    from[i%2] /= 10.;
    step[i%2] /= 10.;
  }
  
  /*
  SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0]);

  commandList[0] = 0;
  commandList[1] = 11;
  commandList[2] = min - 0.05;
  commandList[3] = min + 0.05;
  fFitter->ExecuteCommand("SCAn",commandList,4);  
    
  min = fFitter->GetParameter(0);
  if(fVerboseLevel > 0)
    std::cout << "MIN FOUND AFTER 3rd SCAN = " << min << std::endl;
  
  SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0]);

  commandList[0] = 100000;
  commandList[1] = fitPrecision;
  fFitter->ExecuteCommand(minimizer,commandList,2);
  //fFitter->ExecuteCommand("HESSe",commandList,1);
  */
  
  fFitter->ExecuteCommand("SHOw FCNvalue",commandList,0);

  return true;
}

bool EXORotationAngleFitter::PrepareEnergyVectors()
{
  return PrepareEnergyMC() && PrepareEnergyData();
}

bool EXORotationAngleFitter::PrepareEnergyMC()
{
  fEnergyMC.clear();
  fEnergyWeight.clear();
    
  const std::vector<std::string> inputFileNames = fWeekRunsInfo->GetListOf("MCSelectionFileName");
  const std::vector<std::string> mcIds = fWeekRunsInfo->GetListOf("MCId");
  const std::vector<std::string> multWildCards = fWeekRunsInfo->GetListOf("MCMultiplicityWildCard");

  if(inputFileNames.empty() || (inputFileNames.size() != multWildCards.size() || inputFileNames.size() != mcIds.size()))
  {
    std::cout << "Cannot AddMC, incompatible number files!\n";
    return false;
  }

  std::set<TString> processedMCs;
  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {
    TString mcId = mcIds.at(i);
    mcId = mcId.ReplaceAll(multWildCards.at(i),fMultiplicity.c_str());
    
    if(processedMCs.count(mcId.Data()))
      continue;
    
    //if(fVerboseLevel > 0)
    std::cout << "Adding MC: " << mcId.Data() << std::endl;

    TString inputFileName = inputFileNames.at(i);
    inputFileName = inputFileName.ReplaceAll(multWildCards.at(i),fMultiplicity.c_str());

    TFile inputFile(inputFileName.Data(),"READ");
    if(inputFile.IsZombie())
    {
      std::cerr << "Cannot open file " << inputFileName.Data() << std::endl;
      return false;
    }
      
      bool useTEntryList = true;
      
      TTree *tree;
      if (useTEntryList) {
          TEntryList *list = (TEntryList*)inputFile.Get(Form("EventList_%s",fMultiplicity.c_str()));
          
          TFile *f = new TFile(list->GetFileName(),"READ");
          
          if (f->IsZombie()) {std::cerr << "Cannot open file " << inputFileName.Data() << " from TEntryList" << std::endl; return false;}
          
          tree = (TTree*)f->Get(list->GetTreeName());
          tree->SetEntryList(list);
      }
      else {
          tree = (TTree*) inputFile.Get("mcTree");
      }
      
    if(!tree)
    {
      std::cerr << "Cannot get mcTree in file " << inputFileName.Data() << std::endl;
      return false;
    }
    tree->SetEstimate(tree->GetEntries()+1);
    tree->Draw("energy_mc:weight","","para goff");

    std::vector<double> energyMC(tree->GetSelectedRows());
    std::vector<double> energyWeight(tree->GetSelectedRows());
        
    for(Long64_t j = 0; j < tree->GetSelectedRows(); j++)
    {      
      energyMC[j] = tree->GetV1()[j];
      energyWeight[j] = tree->GetV2()[j];
    }

    fEnergyMC.insert(std::pair<std::string, std::vector<double> >(mcId.Data(), energyMC));  
    fEnergyWeight.insert(std::pair<std::string, std::vector<double> >(mcId.Data(), energyWeight));  
   
    processedMCs.insert(mcId.Data());
  }

  return true;
}

bool EXORotationAngleFitter::PrepareEnergyData()
{ 
  fEnergyCharge.clear();
  fEnergyScint.clear();

  const std::vector<std::string> inputFileNames = fWeekRunsInfo->GetListOf("DataSelectionFileName");
  const std::vector<std::string> mcIds = fWeekRunsInfo->GetListOf("MCId");
  const std::vector<std::string> mcMultWildCards = fWeekRunsInfo->GetListOf("MCMultiplicityWildCard");
  const std::vector<std::string> dataIds = fWeekRunsInfo->GetListOf("DataId");
  const std::vector<std::string> dataMultWildCards = fWeekRunsInfo->GetListOf("DataMultiplicityWildCard");
  //const std::vector<std::string> prescales = fWeekRunsInfo->GetListOf("TriggerPrescale"); 

  if(inputFileNames.empty())
  {
    std::cerr << "Cannot AddData, incompatible number files!\n";
    return false;
  } 
  
  size_t n = inputFileNames.size();
  for(size_t i = 0; i < n; i++)
  {   
    TString mcId = mcIds.at(i);
    mcId = mcId.ReplaceAll(mcMultWildCards.at(i).c_str(),fMultiplicity.c_str());

    TString inputFileName = inputFileNames.at(i);
    inputFileName = inputFileName.ReplaceAll(dataMultWildCards.at(i).c_str(),fMultiplicity.c_str());

    TFile inputFile(inputFileName.Data(),"READ");
    if(inputFile.IsZombie())
    {
      std::cerr << "Cannot open file " << inputFileName.Data() << std::endl;
      return false;
    }
      
      bool useTEntryList = true;
      
      TTree *tree;
      if (useTEntryList) {
          TEntryList *list = (TEntryList*)inputFile.Get(Form("EventList_%s",fMultiplicity.c_str()));
          
          TFile *f = new TFile(list->GetFileName(),"READ");
          
          if (f->IsZombie()) {std::cerr << "Cannot open file " << inputFileName.Data() << " from TEntryList" << std::endl; return false;}
          
          tree = (TTree*)f->Get(list->GetTreeName());
          tree->SetEntryList(list);
      }
      else {
          tree = (TTree*) inputFile.Get("dataTree");
      }
      
    if(!tree)
    {
      std::cerr << "Cannot get dataTree in file " << inputFileName.Data() << std::endl;
      return false;
    }

    tree->SetEstimate(tree->GetEntries()+1);
    tree->Draw("e_charge:e_scint","","para goff");
    std::vector<double> energyCharge(tree->GetSelectedRows());
    std::vector<double> energyScint(tree->GetSelectedRows());
        
    //std::vector<double> e_rotated(tree->GetSelectedRows());
    for(Long64_t j = 0; j < tree->GetSelectedRows(); j++)
    {      
      energyCharge[j] = tree->GetV1()[j];
      energyScint[j] = tree->GetV2()[j];
    }
    
    TString dataId = dataIds.at(i);
    dataId = dataId.ReplaceAll(dataMultWildCards.at(i).c_str(),fMultiplicity.c_str());

    fEnergyCharge.insert(std::pair<std::string, std::vector<double> >(dataId.Data(), energyCharge));  
    fEnergyScint.insert(std::pair<std::string, std::vector<double> >(dataId.Data(), energyScint));  

    if(fVerboseLevel > 0)
      std::cout << "Adding data: " << dataId.Data() << " associated to " << mcId.Data() << std::endl;
  }

  for(std::map<std::string, std::vector<double> >::iterator energy = fEnergyCharge.begin(); energy != fEnergyCharge.end(); energy++)
  {
    if(fEnergyCharge.at(energy->first).empty())
      return false;
    if(fEnergyScint.count(energy->first) != 1)
      return false;
    if(fEnergyCharge.at(energy->first).size() != fEnergyScint.at(energy->first).size())
      return false;
  }

  return true;
}

void EXORotationAngleFitter::SaveOutputFile(std::string name, bool saveBestResolFit, int rebin)
{
  if(name == "")
    name = fOutputFileName;
  
  if(saveBestResolFit)  
  {
    SetFitPars(false,fBestCalibPars[0],fBestCalibPars[1],fBestResolPars[0],fBestResolPars[0]);
    
    RunEnergyFitterWithAngle(fBestAngle,true,"bestFit");
    
    if (!fUseGaussErrfc) {
        TString tempName = name;
        tempName.ReplaceAll(".root","_BestResolFit.root");
        fEnergyMCBasedFitter->SaveHistosIn(tempName.Data(),"RECREATE",rebin);
    }
  }

  TFile rout(name.c_str(),"RECREATE");
  TNtuple tuple("AngleFit","","week:multiplicity:energy:angle:resolution");
  double bestAngle = fBestAngle;
  double bestResol = fBestResol;
  if(fFitter)
  {
    bestAngle = fFitter->GetParameter(0);
    bestResol = fFitter->GetMinuit()->fAmin;
  }
  tuple.Fill((float) fWeekIndexInt, (float) fMultiplicityInt, (float) fMinimizationEnergyValue, (float) bestAngle, (float) bestResol);
  rout.cd();
  tuple.Write();
  if(fFitter)
    fFitter->GetMinuit()->Write(Form("MINUIT_%s_%s",fWeekIndex.c_str(),fMultiplicity.c_str()));
  if(fResolVsAngle1)
    fResolVsAngle1->Write(Form("ResolVsAngle1_%s_%s",fWeekIndex.c_str(),fMultiplicity.c_str()));
  if(fResolVsAngle2)
    fResolVsAngle2->Write(Form("ResolVsAngle2_%s_%s",fWeekIndex.c_str(),fMultiplicity.c_str()));
  if(fResolVsAngle3)
    fResolVsAngle3->Write(Form("ResolVsAngle3_%s_%s",fWeekIndex.c_str(),fMultiplicity.c_str()));
  if(fResolVsAngle4)
    fResolVsAngle4->Write(Form("ResolVsAngle4_%s_%s",fWeekIndex.c_str(),fMultiplicity.c_str()));
  rout.Close();

  std::cout << "Results saved in file " << name << std::endl;

  return;
}
