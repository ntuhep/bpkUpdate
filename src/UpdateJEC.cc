/*******************************************************************************
*
*  Filename    : UpdateJEC.cc
*  Description : Meat of FWLite JEC updating functions
*  Author      : Yi-Mu "Enoch" Chen [ ensc@hep1.phys.ntu.edu.tw ]
*
*******************************************************************************/
#include "bpkFrameWork/bpkUpdate/interface/UpdateObjects.hpp"

#include "CondFormats/JetMETObjects/interface/FactorizedJetCorrector.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectionUncertainty.h"
#include "CondFormats/JetMETObjects/interface/JetCorrectorParameters.h"
#include "JetMETCorrections/Modules/interface/JetResolution.h"
#include "CondFormats/JetMETObjects/interface/JetResolutionObject.h"

#include "TTree.h"
#include "bpkFrameWork/bprimeKit/interface/format.h"

#include <boost/filesystem.hpp>
#include <cstdlib>
#include <iostream>
#include <string>

using namespace std;
namespace opt = boost::program_options;

/*******************************************************************************
*   JEC options parsing
*******************************************************************************/
opt::options_description
JECJEROptions()
{
  opt::options_description desc( "JEC related options" );
  desc.add_options()
    ( "runjec", "Flag to run JEC" )
    ( "runjer", "Flag to run JER" )
    ( "jecversion,j",  opt::value<string>()->default_value(""), "JEC versioning to use" )
    ( "jerversion,r", opt::value<string>()->default_value(""), "JER versioning to use" )
  ;
  return desc;
}

/*******************************************************************************
*   Static helper functions for getting results
*******************************************************************************/
static bool   CheckJECJER( const opt::variables_map& arg );
static string JECVersion( const opt::variables_map& arg );

struct JetChanger
{
  FactorizedJetCorrector*   jec;
  JetCorrectionUncertainty* jecunc;
  JME::JetResolution*            jer;
  JME::JetResolutionScaleFactor* jersf;
};

static JetChanger GetCorrector( const opt::variables_map& arg, const string& jettype );
static void       CorrectJet( JetInfoBranches&, JetChanger&, const double rho );

/******************************************************************************/

void
UpdateJEC( TTree* oldntuple, TTree* newntuple, const opt::variables_map& arg )
{
  if( !CheckJECJER( arg ) ){
    cerr << "Skipping over JEC/JER update" << endl;
    return;
  }

  // Getting jet energy correctors
  // https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookJetEnergyCorrections#JetEnCorFWLite
  // Getting jet energy resolution
  // https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookJetEnergyResolution#Accessing_factors_from_text_file
  JetChanger ak4cor;
  JetChanger ak4puppicor;
  JetChanger ak8puppicor;
  JetInfoBranches ak4jet;
  JetInfoBranches ak4jetpuppi; 
  JetInfoBranches ak8jetpuppi;
  JetInfoBranches ca8jetpuppi;

  if( arg.count( "CHS" ) ){
    ak4cor = GetCorrector( arg, "AK4PFchs" );
    // Setting up CHS Jet info branches
    ak4jet.Register( oldntuple, "JetInfo" );
  }
  if( arg.count( "Puppi" ) ){
    ak4puppicor = GetCorrector( arg, "AK4PFPuppi" );
    // Follow up https://cms-talk.web.cern.ch/t/jet-energy-corrections-for-full-2024-prompt-data-now-available/109472/1
    // where "While the corrections are primarily provided for AK4 jets, they are also applicable to AK8 PUPPI jets.".
    // However, it should be modified as "AK8PFPuppi" if there is a dedicated AK8 correction.
    ak8puppicor = GetCorrector( arg, "AK4PFPuppi" );
    // Setting up Puppi Jet info branches
    ak4jetpuppi.Register( oldntuple, "JetInfoPuppi" );
    ak8jetpuppi.Register( oldntuple, "JetAK8Puppi" );
    ca8jetpuppi.Register( oldntuple, "JetCA8Puppi" );
  }
  EvtInfoBranches evtinfo;                  
  evtinfo.Register( oldntuple, "EvtInfo" );

  for( int i = 0; i <  oldntuple->GetEntries() && i != MaxEvent( arg ); ++i ){
    if( i%ReportEvent(arg) == 0 ){
      cout << "Running event: " <<  i+1 << endl;
    }
    oldntuple->GetEntry( i );
    if( arg.count( "CHS" ) ){
      CorrectJet( ak4jet, ak4cor, evtinfo.Rho );
    }
    if( arg.count( "Puppi" ) ){
      CorrectJet( ak4jetpuppi, ak4puppicor, evtinfo.Rho );
      CorrectJet( ak8jetpuppi, ak8puppicor, evtinfo.Rho );
      CorrectJet( ca8jetpuppi, ak8puppicor, evtinfo.Rho );
    }

    newntuple->Fill();// Adding modified entry to new ntuple
  }
  cout << "Done! "  << endl;

  // Cleaning up
  if( arg.count( "CHS" ) ){
    delete ak4cor.jec;
    delete ak4cor.jecunc;
    delete ak4cor.jer;
    delete ak4cor.jersf;
  } 
  if( arg.count( "Puppi" ) ){
    delete ak4puppicor.jec;
    delete ak4puppicor.jecunc;
    delete ak4puppicor.jer;
    delete ak4puppicor.jersf;
    delete ak8puppicor.jec;
    delete ak8puppicor.jecunc;
    delete ak8puppicor.jer;
    delete ak8puppicor.jersf;
  }

}

/*******************************************************************************
*   Static function implementation
*******************************************************************************/
static const string datadir = getenv( "CMSSW_BASE" ) + string( "/src/bpkFrameWork/bpkUpdate/data/" );

bool
CheckJECJER( const opt::variables_map& arg )
{
  if( arg.count( "runjec" ) ){
    if( !arg.count( "jecversion" ) ){
      cerr << "JEC version not specificed!" << endl;
      cerr << JECJEROptions() << endl;
      throw std::invalid_argument( "Run JEC required but has invalid input" );
    }
    return true;
  } else if( arg.count( "runjer" ) ){
    if( !arg.count( "jerversion" ) ){
      cerr << "JER version not specificed!" << endl;
      cerr << JECJEROptions() << endl;
      throw std::invalid_argument( "Run JER required but has invalid input" );
    }
    return true;
  } else {
    return false;
  }
}

/******************************************************************************/

string
JECVersion( const opt::variables_map& arg )
{
  return arg["jecversion"].as<string>();
}

string
JERVersion( const opt::variables_map& arg )
{
  return arg["jerversion"].as<string>();
}

/******************************************************************************/

JetChanger
GetCorrector( const opt::variables_map& arg, const string& jettype  )
{
  FactorizedJetCorrector* Jec          = nullptr;
  JetCorrectionUncertainty* Jecunc     = nullptr;
  JME::JetResolution* Jer              = nullptr;
  JME::JetResolutionScaleFactor* Jersf = nullptr;

  //JEC 
  const string l1file  = datadir + JECVersion( arg ) + "/" + JECVersion( arg ) + "_L1FastJet_"+ jettype + ".txt";
  const string l2file  = datadir + JECVersion( arg ) + "/" + JECVersion( arg ) + "_L2Relative_"+ jettype + ".txt";
  const string l3file  = datadir + JECVersion( arg ) + "/" + JECVersion( arg ) + "_L3Absolute_"+ jettype + ".txt";
  const string l23file = datadir + JECVersion( arg ) + "/" + JECVersion( arg ) + "_L2L3Residual_"+ jettype + ".txt";
  const string uncfile = datadir + JECVersion( arg ) + "/" + JECVersion( arg ) + "_Uncertainty_"+ jettype + ".txt";

  if( JECVersion( arg ) != "" ){
    if( !boost::filesystem::exists( l1file ) ||
        !boost::filesystem::exists( l2file ) ||
        !boost::filesystem::exists( l3file ) ||
        !boost::filesystem::exists( l23file ) ){
      cerr << "Missing JEC source files! Please check if following files exists!"  << endl
           << "\t" << l1file << endl
           << "\t" << l2file << endl
           << "\t" << l3file << endl
           << "\t" << l23file << endl;
      throw invalid_argument( "Missing file!" );
    } else {

      JetCorrectorParameters ResJetPar( l23file );
      JetCorrectorParameters L3JetPar( l3file );
      JetCorrectorParameters L2JetPar( l2file );
      JetCorrectorParameters L1JetPar( l1file );

      Jec    = new FactorizedJetCorrector( {L1JetPar, L2JetPar, L3JetPar, ResJetPar} );
      Jecunc = new JetCorrectionUncertainty( uncfile );
    }
  }

  //JER
  const string resofile   = datadir + JERVersion( arg ) + "/" + JERVersion( arg ) + "_PtResolution_" + jettype + ".txt";
  const string resosffile = datadir + JERVersion( arg ) + "/" + JERVersion( arg ) + "_SF_" + jettype + ".txt";

  if( JERVersion( arg ) != "" ){
    if( !boost::filesystem::exists( resofile ) ||
        !boost::filesystem::exists( resosffile ) ){
      cerr << "Missing JER source files! Please check if following files exists!"  << endl
           << "\t" << resofile << endl
           << "\t" << resosffile << endl;
      throw invalid_argument( "Missing file!" );
    } else {
      Jer    = new JME::JetResolution( resofile );
      Jersf  = new JME::JetResolutionScaleFactor( resosffile );
    }
  }

  return { Jec, Jecunc, Jer, Jersf };
}


/******************************************************************************/

void
CorrectJet( JetInfoBranches& jetinfo, JetChanger& cor, const double rho )
{
  for( int i = 0; i < jetinfo.Size; ++i ){

    // Getting Jet energy correction and uncertainty
    if (cor.jec != nullptr){
      cor.jec->setJetEta( jetinfo.Eta[i] );
      cor.jec->setJetPhi( jetinfo.Phi[i] );
      cor.jec->setJetPt( jetinfo.PtCorrRaw[i] );
      cor.jec->setJetA( jetinfo.Area[i] );
      cor.jec->setRho( rho );
      jetinfo.Unc[i] = cor.jec->getCorrection();

      cor.jecunc->setJetEta( jetinfo.Eta[i] );
      cor.jecunc->setJetPt( jetinfo.PtCorrRaw[i] * jetinfo.Unc[i] );
      jetinfo.JesUnc[i] = cor.jecunc->getUncertainty(true);
    }

    // Getting jet resolution and uncertainty
    if (cor.jer != nullptr) {
      JME::JetParameters jet_parameters;
      jet_parameters.setJetPt( jetinfo.PtCorrRaw[i] * jetinfo.Unc[i] ).setJetEta( jetinfo.Eta[i] ).setRho( rho );
      jetinfo.JERPt[i] = cor.jer->getResolution( jet_parameters );
      jetinfo.JERScale[i]     = cor.jersf->getScaleFactor( jet_parameters );
      jetinfo.JERScaleUp[i]   = cor.jersf->getScaleFactor( jet_parameters, Variation::UP );
      jetinfo.JERScaleDown[i] = cor.jersf->getScaleFactor( jet_parameters, Variation::DOWN );
    }
  }
}
