
import ROOT

ROOT.gSystem.Load('libEXOCalibUtilities')
ROOT.gSystem.Load('${EXOLIB}/fitting/lib/libEXOFitting')
ROOT.gSystem.Load('../lib/libEXOEnergy.so')

def example():

    ROOT.EXOFiducialVolume().SetUserHexCut(162, 10, 182)
    mult_cut = 'multiplicity > 1.5'
    exo_fitter = ROOT.EXOFitter()
    exo_fitter.SetApplyVetoes(False)
    exo_fitter.SetRandomTrigger(True)

    data_specific_cut = 'e_scint > 0. && e_charge > 0. && energy_ms > 0.'    
    data_cut = exo_fitter.GetDefaultDataCut() + ' && %s && %s' % (mult_cut,data_specific_cut)

    print 'Data cut:', data_cut

    rin = ROOT.TFile('/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_04_10/no_cc_calibration/newmc_2014v1_20130nudenoised_2014v1_run_4439_cpg_4_week_61_weak_co60_s5_px255_py39_nz30_pre_0_tree.root','read')
    tree = rin.Get('dataTree')
    eventSum = ROOT.EXOEventSummary()
    tree.SetBranchAddress('EventSummary',eventSum)

    tree.SetEstimate(tree.GetEntries()+1)
    tree.Draw("eventNum",data_cut,"goff")
    eventList = []
    for i in range(tree.GetSelectedRows()):
        eventList.append(int(tree.GetV1()[i]))

    calib = ROOT.EXOEnergyCalib.GetInstanceForFlavor("2014-v1","2013-0nu-denoised","2014-v1")

    h_energy_ms = ROOT.TH1D("energy_ms","Standard calibration",504,980,3500)
    h_sumcce_ms = ROOT.TH1D("sum_ms","My CC sum calibration",504,980,3500)
    h_ccwise_ms = ROOT.TH1D("ccwise_ms","My CC-wise calibration",504,980,3500)

    for i in range(tree.GetEntries()):
        tree.GetEntry(i)
        if not eventSum.eventNum in eventList:
            continue

        peak = ROOT.EXOCalibManager.GetCalibManager().getCalib(ROOT.EXOThoriumPeakHandler.GetHandlerName(),"2013-0nu-denoised",eventSum.time_sec)
        angle = peak.GetRotationAngle(1)

        ccs = eventSum.cluster_energy
        sum = 0
        for j in range(ccs.size()):
            sum += ccs.at(j)            
        
        print 'Event', eventSum.eventNum, 'e_charge =', eventSum.e_charge, 'sum ccs =', sum, 'angle', angle

        energy_ms = calib.CalibratedRotatedEnergy(eventSum.e_charge,eventSum.e_scint,2,eventSum.time_sec,0)
        sumcce_ms = calib.CalibratedRotatedEnergy(sum,eventSum.e_scint,2,eventSum.time_sec,0)

        ccwise_ene_ms = 0.
        for j in range(ccs.size()):
            cc_ene = calib.CalibratedRotatedEnergy(ccs.at(j),ccs.at(j)/sum * eventSum.e_scint,1,eventSum.time_sec,0)
            #print 'CC energy', cc_ene
            ccwise_ene_ms += cc_ene

        print 'Calib', energy_ms, sumcce_ms, ccwise_ene_ms

        h_energy_ms.Fill(energy_ms)
        h_sumcce_ms.Fill(sumcce_ms)
        h_ccwise_ms.Fill(ccwise_ene_ms)



    h_energy_ms.Draw()
    h_sumcce_ms.SetLineColor(2)
    h_sumcce_ms.Draw('sames')
    h_ccwise_ms.SetLineColor(4)
    h_ccwise_ms.Draw('sames')

    raw_input('check plots')

    rmc = ROOT.TFile('/u/xo/licciard/my_exodir/for_ruey/dn_0nu_thres980_calib_campaign_4_week_62_source_co60_th228_pos_s5_pre_-1_0_1_fv_162_10_182_fit.root','read')
    hmc = rmc.Get('histo_scaling_smeared_energy_mc_ms_final_denoised_data_week_62_63_source_co60_pos_s5_pre_0_run_4439;1').Clone('mc')

    print h_energy_ms.Integral(1,h_energy_ms.GetNbinsX()),hmc.Integral()

    hmc.Scale(h_energy_ms.Integral(1,h_energy_ms.GetNbinsX())/hmc.Integral())

    rout = ROOT.TFile('/u/xo/licciard/my_exodir/for_ruey/test_ClusterCalib.root','recreate')
    rout.cd()
    hmc.Write()
    h_energy_ms.Write()
    h_sumcce_ms.Write()
    h_ccwise_ms.Write()
    rout.Close()

    rmc.Close()

    rin.Close()

def example2():

    infoRuns = ROOT.EXOSourceRunsPolishedInfo('../data/UpdatedLivetimeLJSourceRunsInfo.txt')

    infoRuns.SetDataTreeFileNames('newmc_2014v1_20130nudenoised_2014v1','/nfs/slac/g/exo_data4/users/Energy/data/WIPP/preprocessed/2014_04_22_rhel6/no_src_activity/')
    infoRuns.SetDataSelectionFileNames(cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/WIPP/selection/2014_04_22_calib_rhel6/from_no_src_activity/','[MULTIPLICITY]')

    infoRuns.SetMCTreeFileNames('/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/preprocessed/2014_04_10/') 
    infoRuns.SetMCSelectionFileNames(cutName,'/u/xo/licciard/my_exodir/exo_data4_users/Energy/data/MC/selection/2014_04_14_calib/','[MULTIPLICITY]')

    infoRuns.CutExact('SourceName','Co-60')
    infoRuns.CutExact('SourcePositionS','5')

    runFileNames = infoRuns.GetListOf('')

    




if __name__ == "__main__":

    example()
