/*******************************************************************************
*
*  Filename    : bkgUpdateJEC.cc
*  Description : User interface for updating JEC/JER variables
*  Author      : Yi-Mu "Enoch" Chen [ ensc@hep1.phys.ntu.edu.tw ]
*
*******************************************************************************/
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/parsers.hpp>
#include <boost/program_options/variables_map.hpp>
#include <iostream>
#include <string>
#include <vector>

#include "bpkFrameWork/bpkUpdate/interface/UpdateObjects.hpp"

#include "TChain.h"
#include "TFile.h"
#include "TTree.h"

using namespace std;
namespace opt = boost::program_options;

int
main( int argc, char* argv[] )
{
  opt::options_description desc( "Options for updating bprimeKit ntuples" );
  desc.add( CommonOptions() ).add( JECJEROptions() );

  opt::variables_map arg;
  try {
    arg = CheckCommon( argc, argv, desc );
  } catch( ... ){
    return 1;
  }

  /*******************************************************************************
  *******************************************************************************/
  // Making old version of ntuples
  TChain* oldntuple     = new TChain( "bprimeKit/root" );

  for( const auto& file : InputFiles( arg ) ){
    oldntuple->Add( file.c_str() );
  }

  /*******************************************************************************
  *******************************************************************************/
  // Creating new file and tree
  TFile* newfile   = TFile::Open( OutputFile( arg ).c_str(), "RECREATE" );
  newfile->mkdir("bprimeKit");
  newfile->cd("bprimeKit");


  /******************************************************************************/
  /******************************************************************************/
  // Event tree updating
  TTree* newntuple = oldntuple->CloneTree( 0 );

  try {
    UpdateJEC( oldntuple, newntuple, arg );
  } catch( ... ){
    return 1;
  }
  // Saving newntuple to file
  newntuple->AutoSave();

  // Trusting ROOT to autoclean the pointer objects.
  return 0;
}
