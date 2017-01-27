/*******************************************************************************
*
*  Filename    : UpdateObjects.hpp
*  Description : List of available function functions (implemented in src)
*  Author      : Yi-Mu "Enoch" Chen [ ensc@hep1.phys.ntu.edu.tw ]
*
*******************************************************************************/
#ifndef BPKFRAMEWORK_BPKUPDATE_UPDATEOBJECTS
#define BPKFRAMEWORK_BPKUPDATE_UPDATEOBJECTS

#include "TTree.h"
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <string>
#include <vector>

// Common options for any update
boost::program_options::options_description CommonOptions();
boost::program_options::variables_map       CheckCommon(
  int argc, char* argv[], const boost::program_options::options_description& );

// Access of common functions
std::vector<std::string> InputFiles( const boost::program_options::variables_map& );
std::string              OutputFile( const boost::program_options::variables_map& );
int                      MaxEvent(   const boost::program_options::variables_map& );

// Specific options for JEC related updates
boost::program_options::options_description JECOptions();
void                                        UpdateJEC( TTree* oldntuple, TTree* newntuple, const boost::program_options::variables_map& );

#endif/* end of include guard: BPKFRAMEWORK_BPKUPDATE_UPDATEOBJECTS */
