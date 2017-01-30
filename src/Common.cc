/*******************************************************************************
*
*  Filename    : Common.cc
*  Description : Common setting for master boost program options
*  Author      : Yi-Mu "Enoch" Chen [ ensc@hep1.phys.ntu.edu.tw ]
*
*******************************************************************************/
#include "bpkFrameWork/bpkUpdate/interface/UpdateObjects.hpp"

#include <boost/program_options/parsers.hpp>
#include <string>
#include <vector>
#include <iostream>

using namespace std;
namespace opt = boost::program_options;


/******************************************************************************/

opt::options_description
CommonOptions()
{
  opt::options_description common( "Common options" );
  common.add_options()
    ( "input,i", opt::value<vector<string> >()->multitoken(), "List of input files to read" )
    ( "output,o", opt::value<string>(), "Output file to store results" )
    ( "maxevent,m", opt::value<int>()->default_value(10000), "Maximum number of events to process" )
    ( "report,r", opt::value<int>()->default_value(1000), "Report every r events" )
    ( "help,h", "Print help message" )
  ;
  return common;
}

/******************************************************************************/

opt::variables_map
CheckCommon(
  int argc, char* argv[],
  const opt::options_description& desc )
{
  opt::variables_map arg;
  opt::store( opt::parse_command_line( argc, argv, desc ), arg );
  opt::notify( arg );

  if( arg.count( "help" ) ){
    cerr << desc << endl;
    throw "Printing help requested!";
  }
  if( !arg.count( "input"  ) ){
    cerr << "Input file(s) not specified!" << endl << CommonOptions() << endl;
    throw std::invalid_argument("No input");
  }
  if( !arg.count( "output" ) ){
    cerr << "Output file not specified!"   << endl << CommonOptions() << endl;
    throw std::invalid_argument("No output");
  }

  return arg;
}


/******************************************************************************/
vector<string>  InputFiles( const opt::variables_map& arg )
{
  return arg["input"].as<vector<string>>();
}

/******************************************************************************/

string OutputFile( const opt::variables_map& arg )
{
  return arg["output"].as<string>();
}


/******************************************************************************/

int MaxEvent( const opt::variables_map& arg )
{
  return arg["maxevent"].as<int>();
}

/******************************************************************************/

int ReportEvent( const opt::variables_map& arg )
{
  return arg["report"].as<int>();
}
