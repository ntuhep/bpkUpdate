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
JECOptions()
{
  opt::options_description desc( "JEC related options" );
  desc.add_options()
    ( "runjec", "Flag to run JEC/JER" )
    ( "jecversion,j", opt::value<string>(), "JEC/JER versioning to use" )
  ;
  return desc;
}

/*******************************************************************************
*   Static helper functions for getting results
*******************************************************************************/
static bool   CheckJEC( const opt::variables_map& arg );
static string JECVersion( const opt::variables_map& arg );

struct JetChanger
{
  FactorizedJetCorrector*   jec;
  JetCorrectionUncertainty* jecunc;
  // JME::JetResolution            jetres;
  // JME::JetResolutionScaleFactor jetressf;
};

static JetChanger GetCorrector( const opt::variables_map& arg, const string& jettype );
static void       CorrectJet( JetInfoBranches&, JetChanger&, const double rho );

/******************************************************************************/

void
UpdateJEC( TTree* oldntuple, TTree* newntuple, const opt::variables_map& arg )
{
  if( !CheckJEC( arg ) ){
    cerr << "Skipping over JEC/JER update" << endl;
    return;
  }

  // Getting jet energy correctors
  // https://twiki.cern.ch/twiki/bin/view/CMSPublic/WorkBookJetEnergyCorrections#JetEnCorFWLite
  JetChanger ak4cor = GetCorrector( arg, "AK4PFchs" );
  JetChanger ak8cor = GetCorrector( arg, "AK8PFchs" );

  // Setting up Jet info branches
  JetInfoBranches ak4jet;
  ak4jet.Register( oldntuple, "JetInfo" );
  JetInfoBranches ak8jet;
  ak8jet.Register( oldntuple, "JetAK8Info" );
  JetInfoBranches ca8jet;
  ca8jet.Register( oldntuple, "JetCA8Info" );
  EvtInfoBranches evtinfo;
  evtinfo.Register( oldntuple, "EvtInfo" );

  for( int i = 0; i <  oldntuple->GetEntries() && i != MaxEvent( arg ); ++i ){
    cout << "\rRunning event: " <<  i+1 << flush;
    oldntuple->GetEntry( i );

    CorrectJet( ak4jet, ak4cor, evtinfo.Rho );
    CorrectJet( ak8jet, ak8cor, evtinfo.Rho );
    CorrectJet( ca8jet, ak8cor, evtinfo.Rho );

    newntuple->Fill();// Adding modified entry to new ntuple
  }
  cout << "Done! "  << endl;

  // Cleaning up
  delete ak4cor.jec;
  delete ak4cor.jecunc;
  delete ak8cor.jec;
  delete ak8cor.jecunc;

}

/*******************************************************************************
*   Static function implementation
*******************************************************************************/
static const string datadir = getenv( "CMSSW_BASE" ) + string( "/src/bpkFrameWork/bpkUpdate/data/" );

bool
CheckJEC( const opt::variables_map& arg )
{
  if( arg.count( "runjec" ) ){
    if( !arg.count( "jecversion" ) ){
      cerr << "JEC/JER version not specificed!" << endl;
      cerr << JECOptions() << endl;
      throw std::invalid_argument( "Run JEC required but has invalid input" );
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

/******************************************************************************/

JetChanger
GetCorrector( const opt::variables_map& arg, const string& jettype  )
{
  const string l1file  = datadir + JECVersion( arg ) + "_L1FastJet_"+ jettype + ".txt";
  const string l2file  = datadir + JECVersion( arg ) + "_L2Relative_"+ jettype + ".txt";
  const string l3file  = datadir + JECVersion( arg ) + "_L3Absolute_"+ jettype + ".txt";
  const string l23file = datadir + JECVersion( arg ) + "_L2L3Residual_"+ jettype + ".txt";
  const string uncfile = datadir + JECVersion( arg ) + "_Uncertainty_"+ jettype + ".txt";
  if( !boost::filesystem::exists( l1file ) ||
      !boost::filesystem::exists( l2file ) ||
      !boost::filesystem::exists( l3file ) ||
      !boost::filesystem::exists( l23file ) ){
    cerr << "Missing files! Please check if following files exists!"  << endl
         << "\t" << l1file << endl
         << "\t" << l2file << endl
         << "\t" << l3file << endl
         << "\t" << l23file << endl;
    throw invalid_argument( "Missing file!" );
  }
  JetCorrectorParameters ResJetPar( l23file );
  JetCorrectorParameters L3JetPar( l3file );
  JetCorrectorParameters L2JetPar( l2file );
  JetCorrectorParameters L1JetPar( l1file );

  return {
           new FactorizedJetCorrector( {L1JetPar, L2JetPar, L3JetPar, ResJetPar} ),
           new JetCorrectionUncertainty( uncfile )
  };
}


/******************************************************************************/

void
CorrectJet( JetInfoBranches& jetinfo, JetChanger& cor, const double rho )
{
  for( int i = 0; i < jetinfo.Size; ++i ){
    // Getting Jet energy correction
    cor.jec->setJetEta( jetinfo.Eta[i] );
    cor.jec->setJetPt( jetinfo.Pt[i] );
    cor.jec->setJetA( jetinfo.Area[i] );
    cor.jec->setRho( rho );
    jetinfo.Unc[i] = cor.jec->getCorrection();

    // Getting jet energy correction uncertainty
    cor.jecunc->setJetEta( jetinfo.Eta[i] );
    cor.jecunc->setJetPt( jetinfo.Pt[i] * jetinfo.Unc[i] );
    jetinfo.JesUnc[i] = cor.jecunc->getUncertainty(true);
  }
}
