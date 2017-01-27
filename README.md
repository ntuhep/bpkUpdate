# bpkUpdate

A utility for updating contents in existing bprimeKit ntuples files.

## Installation

1. The code must be in the neighboring directory to a existing `bprimeKit` installation. So this installation assumes that the directory `CMSSW_X_Y_Z/src/bpkFrameWork/bprimeKit` and its contents already exists and are up to date. (There is no need to upgrade the existing CMSSW version to the latest version, since we will be reading from text files)

       cd CMSSW_X_Y_Z/src/
       cmsenv

       git clone https://github.com/ntuhep/bpkUpdate.git bpkFrameWork/bpkUpdate

       scram b

2. Getting the require JEC/JER text files in a flat format (no other directories) in the `bpkUpdate/data` directory. Here is a quick Copy&Paste recipe for the JER/JEC Summer16V3 data update.

       cd bpkFrameWork/bpkUpdate/data
       wget https://github.com/cms-jet/JECDatabase/raw/master/tarballs/Summer16_23Sep2016BCDV3_DATA.tar.gz
       wget https://github.com/cms-jet/JECDatabase/raw/master/tarballs/Summer16_23Sep2016EFV3_DATA.tar.gz
       wget https://github.com/cms-jet/JECDatabase/raw/master/tarballs/Summer16_23Sep2016GV3_DATA.tar.gz
       wget https://github.com/cms-jet/JECDatabase/raw/master/tarballs/Summer16_23Sep2016HV3_DATA.tar.gz
       for file in *.tar.gz ; tar -zxvf $file
       mv textFiles/*/*.txt ./
       rm -rf textFiles *.tar.gz


## Running the code
After compiling, we have access to the new command `bpkUpdate`. The command current is intended to run with the following format:
```
bpkUpdate
    --input old1.root old2.root # Accept multiple or single files
    --output /tmp/update.root   # Path to output file
    --maxevent 1000             # Defaults to 100, set to -1 to run all
    --runjec                    # Required for JEC/JER update
    --jecversion Summer16_23Sep2016HV3_DATA # Or similar, must match filename prefixes found in the data directory
```
A single line copy and paste version of the command with abbreviated options
```
bpkUpdate --input bpk_ntuple_old.root -o bpk_ntuple_new.root  -m 1000  --runjec -j Summer16_23Sep2016BCDV3_DATA
```

### Tips and tricks
1. You can use the shell expansion to glob all file in an entire directory for the input. For examples:

      bpkUpdate -i /my/directory/*.root ...
