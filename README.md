# qn-crmd-data-processor
wip for QuarkNet's Cosmic Ray Muon Detector data processing


## Steps for processing data:
### I: Extracting data from DAQ file
1) Install Cygwin, XLaunch, and Octave on computer.
2) Get three files: extractDAQ.C, extractDAQ.sh, and ShowerXX.txt
3) Using Cygwin, navigate to data folder. 
4) Run the command "./extractDAQ.sh [folder/filename]", 
5) The new file [filename].dmp is now created in the folder as determined. 

### II: Processing Lifetime AND Magnetic Moment graph
12) In Cygwin, navigate to the correct folder. 
13) Run command "bash Moment.sh [filename.txt]"
14) The file [filename].mgm should now be created.
15) Run Octave as before.
16) Concatanate all .mgm files that you want to process together. You can use cat [file1].mgm [file2].mgm [file3].mgm [filen].mgm > [allFiles].mgm
16) In file plotmoment.m, change the parameter DAQfileList to [allFiles].mgm.  
17) Run command "run("plotmoment.m")
