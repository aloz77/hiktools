# hiktools

Tools for Hikvision IP cameras

## extract_video.c

    Read/Extract Hikvision Video File Storage  
    Alexey Ozerov (c) 2014 - ver. 0.3

    Options
     -? -h            Display this help
     -i <path>        Input directory path
     -o <path>        Output directory path
     -s <string>      List/extract only file names including string
     -k               Don't overwrite existing output files
     -l               List only, don't extract data
     -t               Only calculate and show totals
     -v               Verbose mode
    
    Compile:
    git clone https://github.com/aloz77/hiktools
    cd hiktools
    ./cc
    
    Test:
    ./extract_video -i /media/ipcam/datadir0/ -o /path/to/output -l -s 2014-10-13 -v

