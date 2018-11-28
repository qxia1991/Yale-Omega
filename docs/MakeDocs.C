{
  // run from the docs folder
  gROOT->Reset();

  gSystem->Exec("rm -rf htmlout");
 
  gSystem->Load("libEXOUtilities");
  THtml h;
  h.SetInputDir("../src/");

  if(gSystem->Load("../lib/libEXOEnergy") < 0) {
   std::cout<<"Could not load the EXOEnergy library. Run \"make\" first!"<<std::endl;
   gSystem->Exit(0);
  };

  h.SetHeader("header.html");//load our custmo header
  h.SetFooter("footer.html");
  h.SetOutputDir("htmlout");
  h.SetProductName("EXOEnergy !!");
  h.SetHomepage("http://exo-data.slac.stanford.edu/");
  Int_t elevel = gErrorIgnoreLevel;//to suppress the annoying warnings
  gErrorIgnoreLevel = kError;
  h.MakeAll(true,"EXO*");
  //gErrorIgnoreLevel = elevel;//restore original (not really needed since the last line)

}
