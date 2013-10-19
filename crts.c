#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <complex.h>
#include <liquid/liquid.h>
#include <time.h>
#include <string.h>
// For Threading (POSIX Threads)
#include <pthread.h>
// For config file
#include <libconfig.h>

//TCP Header Files
#include <sys/socket.h> // for socket(), connect(), send(), and recv() 
#include <sys/types.h>
#include <arpa/inet.h>  // for sockaddr_in and inet_addr() 
#include <string.h>     // for memset() 
#include <unistd.h>     // for close() 
#include <errno.h>
#define PORT 1353
#define MAXPENDING 5


struct CognitiveEngine {
    char modScheme[20];
    char crcScheme[20];
    char innerFEC[20];
    char outerFEC[20];
    float default_tx_power;
    char option_to_adapt[20];
    char goal[20];
    float threshold;
    float latestGoalValue;
    int iterations;
    int payloadLen;
    unsigned int numSubcarriers;
    unsigned int CPLen;
    unsigned int taperLen;

};

struct Scenario {
    int addNoise; //Does the Scenario have noise?
    float noiseSNR;
    float noiseDPhi;
    
    int addInterference; // Does the Scenario have interference?
    
    int addFading; // Does the Secenario have fading?
    float fadeK;
    float fadeFd;
    float fadeDPhi;
};

int readScMasterFile(char scenario_list[30][60])
{
    config_t cfg;                   // Returns all parameters in this structure 
    config_setting_t *setting;
    const char *str;                // Stores the value of the String Parameters in Config file
    int tmpI;                       // Stores the value of Integer Parameters from Config file
    double tmpD;                

    char current_sc[30];
    int no_of_scenarios=1;
    int i;
    char tmpS[30];
    //Initialization
    config_init(&cfg);
   
   
    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg,"master_config_file.txt"))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        printf("\nCould not find master file\n");
        config_destroy(&cfg);
        return -1;
    }
    else
        printf("Found master config file\n");

  
    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        printf("\nFile Type: %s", str);
    else
        printf("\nNo 'filename' setting in configuration file.");

    // Read the parameter group
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        
        if (config_setting_lookup_int(setting, "NumberofScenarios", &tmpI))
        {
            no_of_scenarios=tmpI;
            printf ("\n%d",tmpI);
        }
        
       for (i=1;i<=no_of_scenarios;i++)
       {
         strcpy (current_sc,"scenario_");
         sprintf (tmpS,"%d",i);
         printf ("\n Scenario Number =%s", tmpS);
         strcat (current_sc,tmpS);
         printf ("\n CURRENT SCENARIO =%s", current_sc);
         if (config_setting_lookup_string(setting, current_sc, &str))
          {
              strcpy((scenario_list)+i-1,str);          
              printf ("\nSTR=%s\n",str);
          }
        /*else
            printf("\nNo 'param2' setting in configuration file.");
          */
        printf ("Scenario File:%s\n", scenario_list[i]);
        } 
    }
    config_destroy(&cfg);
    return -1;
} // End readScMasterFile()

int readCEMasterFile(char cogengine_list[30][60])
{
    config_t cfg;               // Returns all parameters in this structure 
    config_setting_t *setting;
    const char *str;            // Stores the value of the String Parameters in Config file
    int tmpI;                   // Stores the value of Integer Parameters from Config file
    double tmpD;                

    char current_ce[30];
    int no_of_cogengines=1;
    int i;
    char tmpS[30];
    //Initialization
    config_init(&cfg);
   
    printf ("\nInside readCEMasterFile function\n");
    printf ("%sCogEngine List[0]:\n",cogengine_list[0] );

    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg,"master_cogengine_file.txt"))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        printf("\nCould not find master file\n");
        config_destroy(&cfg);
        return -1;
    }
    else
        printf("Found master config file\n");
  
    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        printf("\nFile Type: %s", str);
    else
        printf("\nNo 'filename' setting in configuration file.");

    // Read the parameter group
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        if (config_setting_lookup_int(setting, "NumberofCogEngines", &tmpI))
        {
            no_of_cogengines=tmpI;
            printf ("\n%d",tmpI);
        }
        
       for (i=1;i<=no_of_cogengines;i++)
       {
         strcpy (current_ce,"cogengine_");
         sprintf (tmpS,"%d",i);
         //printf ("\n Scenario Number =%s", tmpS);
         strcat (current_ce,tmpS);
         //printf ("\n CURRENT SCENARIO =%s", current_sc);
         if (config_setting_lookup_string(setting, current_ce, &str))
          {
              strcpy((cogengine_list)+i-1,str);          
              printf ("\nSTR=%s\n",str);
          }
        /*else
            printf("\nNo 'param2' setting in configuration file.");
          */
        printf ("Cognitive Engine File:%s\n", cogengine_list[i]);
        } 
    }
    config_destroy(&cfg);
    return -1;
} // End readCEMasterFile()


///////////////////Cognitive Engine///////////////////////////////////////////////////////////
////////Reading the cognitive radio parameters from the configuration file////////////////////
int readCEConfigFile(struct CognitiveEngine * ce,char *current_cogengine_file)
{
    config_t cfg;               // Returns all parameters in this structure 
    config_setting_t *setting;
    const char * str;           // Stores the value of the String Parameters in Config file
    int tmpI;                   // Stores the value of Integer Parameters from Config file
    double tmpD;                


    //Initialization
    config_init(&cfg);

    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg,current_cogengine_file))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        printf("\nFile Type: %s", str);
    else
        printf("\nNo 'filename' setting in configuration file.");

    // Read the parameter group
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        // Read the string
        if (config_setting_lookup_string(setting, "option_to_adapt", &str))
        {
            strcpy(ce->option_to_adapt,str);
            printf ("%s",str);
        }
        /*else
            printf("\nNo 'param2' setting in configuration file.");
            */
       
        if (config_setting_lookup_string(setting, "goal", &str))
        {
            strcpy(ce->goal,str);
            printf ("%s",str);
        }
        if (config_setting_lookup_string(setting, "modScheme", &str))
        {
            strcpy(ce->modScheme,str);
            printf ("Modulation Scheme:%s",str);
        }
        if (config_setting_lookup_string(setting, "crcScheme", &str))
        {
            strcpy(ce->crcScheme,str);
            printf ("CRC Scheme:%s",str);
        }
        if (config_setting_lookup_string(setting, "innerFEC", &str))
        {
            strcpy(ce->innerFEC,str);
            printf ("Inner FEC Scheme:%s",str);
        }
        if (config_setting_lookup_string(setting, "outerFEC", &str))
        {
            strcpy(ce->outerFEC,str);
            printf ("Outer FEC Scheme:%s",str);
        }

        // Read the integers
        if (config_setting_lookup_int(setting, "iterations", &tmpI))
        {
           ce->iterations=tmpI;
           printf("\nIterations: %d", tmpI);
        }
        if (config_setting_lookup_int(setting, "payloadLen", &tmpI))
        {
           ce->payloadLen=tmpI; 
           printf("\nPayloadLen: %d", tmpI);
        }
        if (config_setting_lookup_int(setting, "numSubcarriers", &tmpI))
        {
           ce->numSubcarriers=tmpI; 
           printf("\nNumber of Subcarriers: %d", tmpI);
        }
        if (config_setting_lookup_int(setting, "CPLen", &tmpI))
        {
           ce->CPLen=tmpI; 
           printf("\nCPLen: %d", tmpI);
        }
        if (config_setting_lookup_int(setting, "taperLen", &tmpI))
        {
           ce->payloadLen=tmpI; 
           printf("\nPayloadLen: %d", tmpI);
        }
        // Read the floats
        if (config_setting_lookup_float(setting, "default_tx_power", &tmpD))
        {
           ce->default_tx_power=tmpD; 
           printf("\nDefault Tx Power: %f", tmpD);
        }
        if (config_setting_lookup_float(setting, "latestGoalValue", &tmpD))
        {
           ce->latestGoalValue=tmpD; 
           printf("\nLatest Goal Value: %f", tmpD);
        }
        if (config_setting_lookup_float(setting, "threshold", &tmpD))
        {
           ce->threshold=tmpD; 
           printf("\nThreshold: %f", tmpD);
        }
     }
    config_destroy(&cfg);
     return 1;
} // End readCEConfigFile()

// Default parameters for a Cognitive Engine
struct CognitiveEngine CreateCognitiveEngine() {
    struct CognitiveEngine ce = {
        .default_tx_power = 10.0,
        .threshold = 1.0,                 // Desired value for goal
        .latestGoalValue = 0.0,           // Value of goal to be compared to threshold
        .iterations = 100,                // Number of transmissions made before attemting to receive them.
        .payloadLen = 120,                // Length of payload in frame generation
        .numSubcarriers = 64,             // Number of subcarriers for OFDM
        .CPLen = 16,                      // Cyclic Prefix length
        .taperLen = 4                     // Taper length
    };
        strcpy(ce.modScheme, "QPSK");
        strcpy(ce.option_to_adapt, "mod_scheme");
        strcpy(ce.goal, "payload_valid");
        strcpy(ce.crcScheme, "none");
        strcpy(ce.innerFEC, "Hamming128");
        strcpy(ce.outerFEC, "none");
    return ce;
    // TODO: Call function to read config file and change specified parameters. e.g.
    // ReadCEConfig(&ce);
}

int readScConfigFile(struct Scenario * sc, char *current_scenario_file)
{
    config_t cfg;               // Returns all parameters in this structure 
    config_setting_t *setting;
    //const char *str1, *str2;
    //char str[30];
    const char * str;
    //int tmp,tmp2,tmp3,tmp4,tmp5;
    int tmpI;
    int i;
    char scFileLocation[60];

    printf("In readScConfigFile(): string current_scenario_file: \n%s\n", current_scenario_file);

    // Because the file is in the folder 'scconfigs'
    strcpy(scFileLocation, "scconfigs/");
    strcat(scFileLocation, current_scenario_file);
    printf("In readScConfigFile(): string scFileLocation: \n%s\n", scFileLocation);

    // Initialization 
    config_init(&cfg);

    // Read the file. If there is an error, report it and exit. 
    if (!config_read_file(&cfg, scFileLocation))
    {
        printf("\n%s:%d - %s", config_error_file(&cfg), config_error_line(&cfg), config_error_text(&cfg));
        config_destroy(&cfg);
        return -1;
    }

    // Get the configuration file name. 
    if (config_lookup_string(&cfg, "filename", &str))
        printf("\nFile Type: %s", str);
    else
        printf("\nNo 'filename' setting in configuration file.");

    // Read the parameter group.
    setting = config_lookup(&cfg, "params");
    if (setting != NULL)
    {
        // Read the integer
        if (config_setting_lookup_int(setting, "addNoise", &tmpI))
        {
            printf("\nAddnoise: %d", tmpI);
            sc->addNoise=tmpI;
        }
        else
            printf("\nNo AddNoise setting in configuration file.");
        
        // Read the integer
        if (config_setting_lookup_int(setting, "noiseSNR", &tmpI))
        {
            printf("\nNoise SNR: %d", tmpI);
            sc->noiseSNR=tmpI;
        }
        else
            printf("\nNo Noise SNR setting in configuration file.");
       
        // Read the integer
        if (config_setting_lookup_int(setting, "noiseDPhi", &tmpI))
        {
            sc->noiseDPhi=tmpI;
            printf("\nNoiseDPhi: %d", tmpI);
        }
        else
            printf("\nNo NoiseDPhi setting in configuration file.");

        // Read the integer
        if (config_setting_lookup_int(setting, "addInterference", &tmpI))
        {
            sc->addInterference=tmpI;
            printf("\naddInterference: %d", tmpI);
        }
        else
            printf("\nNo addInterference setting in configuration file.");

        // Read the integer
        if (config_setting_lookup_int(setting, "addFading", &tmpI))
        {
            sc->addFading=tmpI;
            printf("\naddFading: %d", tmpI);
        }
        else
            printf("\nNo addFading setting in configuration file.");

        if (config_setting_lookup_int(setting, "fadeK", &tmpI))
        {
            sc->addFading=tmpI;
            printf("\naddFading: %d", tmpI);
        }
        else
            printf("\nNo addFading setting in configuration file.");

        if (config_setting_lookup_int(setting, "fadeFd", &tmpI))
        {
            sc->addFading=tmpI;
            printf("\naddFading: %d", tmpI);
        }
        else
            printf("\nNo addFading setting in configuration file.");

        if (config_setting_lookup_int(setting, "fadeDPhi", &tmpI))
        {
            sc->addFading=tmpI;
            printf("\naddFading: %d", tmpI);
        }
        else
            printf("\nNo addFading setting in configuration file.");
    printf("\n");
    }

    config_destroy(&cfg);
    return 1;
}

// Default parameter for Scenario
struct Scenario CreateScenario() {
    struct Scenario sc = {
        .addNoise = 1,
        .noiseSNR = 7.0f, // in dB
        .noiseDPhi = 0.001f,

        .addInterference = 0,

        .addFading = 0,
        .fadeK = 30.0f,
        .fadeFd = 0.2f,
        .fadeDPhi = 0.001f
    };
    return sc;
} // End CreateScenario()

// Add AWGN
void addAWGN(float complex * transmit_buffer, struct CognitiveEngine ce, struct Scenario sc)
{
    //options
    float dphi  = sc.noiseDPhi;                              // carrier frequency offset
    float SNRdB = sc.noiseSNR;                               // signal-to-noise ratio [dB]
    unsigned int symbol_len = ce.numSubcarriers + ce.CPLen;  // defining symbol length

    // noise parameters
    float nstd = powf(10.0f, -SNRdB/20.0f); // noise standard deviation
    float phi = 0.0f;                       // channel phase
    
    unsigned int i;

    // noise mixing
    for (i=0; i<symbol_len; i++) {
        transmit_buffer[i] *= cexpf(_Complex_I*phi); // apply carrier offset
        phi += dphi;                                 // update carrier phase
        cawgn(&transmit_buffer[i], nstd);            // add noise
    }
} // End addAWGN()

// Add Rice-K Fading
void addRiceFading(float complex * transmit_buffer, struct CognitiveEngine ce, struct Scenario sc)
{
    // options
    unsigned int symbol_len = ce.numSubcarriers + ce.CPLen; // defining symbol length
    unsigned int h_len;                                     // doppler filter length
    if (symbol_len > 94){
        h_len = 0.0425*symbol_len;
    }
    else {
        h_len = 4;
    }
    float fd           = sc.fadeFd; // maximum doppler frequency
    float K            = sc.fadeK;  // Rice fading factor
    float omega        = 1.0f;      // mean power
    float theta        = 0.0f;      // angle of arrival
    float dphi = sc.fadeDPhi;       // carrier frequency offset
    float phi = 0.0f;               // channel phase

    // validate input
    if (K < 1.5f) {
        fprintf(stderr,"error: fading factor K must be greater than 1.5\n");
        exit(1);
    } else if (omega < 0.0f) {
        fprintf(stderr,"error: signal power Omega must be greater than zero\n");
        exit(1);
    } else if (fd <= 0.0f || fd >= 0.5f) {
        fprintf(stderr,"error: Doppler frequency must be in (0,0.5)\n");
        exit(1);
    } else if (symbol_len== 0) {
        fprintf(stderr,"error: number of samples must be greater than zero\n");
        exit(1);
    }
 
    unsigned int i;

    // allocate array for output samples
    float complex * y = (float complex*) malloc(symbol_len*sizeof(float complex));
    // generate Doppler filter coefficients
    float h[h_len];
    liquid_firdes_doppler(h_len, fd, K, theta, h);

    // normalize filter coefficients such that output Gauss random
    // variables have unity variance
    float std = 0.0f;
    for (i=0; i<h_len; i++)
        std += h[i]*h[i];
    std = sqrtf(std);
    for (i=0; i<h_len; i++)
        h[i] /= std;

    // create Doppler filter from coefficients
    firfilt_crcf fdoppler = firfilt_crcf_create(h,h_len);

    // generate complex circular Gauss random variables
    float complex v;    // circular Gauss random variable (uncorrelated)
    float complex x;    // circular Gauss random variable (correlated w/ Doppler filter)
    float s   = sqrtf((omega*K)/(K+1.0));
    float sig = sqrtf(0.5f*omega/(K+1.0));
    for (i=0; i<symbol_len; i++) {
        // generate complex Gauss random variable
        crandnf(&v);

        // push through Doppler filter
        firfilt_crcf_push(fdoppler, v);
        firfilt_crcf_execute(fdoppler, &x);

        // convert result to random variable with Rice-K distribution
        y[i] = _Complex_I*( cimagf(x)*sig + s ) +
                          ( crealf(x)*sig     );
    }
  for (i=0; i<symbol_len; i++) {
        transmit_buffer[i] *= cexpf(_Complex_I*phi);  // apply carrier offset
        phi += dphi;                                  // update carrier phase
        transmit_buffer[i] *= y[i];                   // apply Rice-K distribution
    }

    // destroy filter object
    firfilt_crcf_destroy(fdoppler);

    // clean up allocated arrays
    free(y);
} // End addRiceFading()

// Enact Scenario
void enactScenario(float complex * transmit_buffer, struct CognitiveEngine ce, struct Scenario sc)
{
    // Check AWGN
    if (sc.addNoise == 1){
       addAWGN(transmit_buffer, ce, sc);
    }
    if (sc.addInterference == 1){
       printf("Warning: There is currently no interference function");
       // Interference function
    }
    if (sc.addFading == 1){
       addRiceFading(transmit_buffer, ce, sc);
    }
    if ( (sc.addNoise == 0) && (sc.addInterference == 0) && (sc.addFading == 0) ){
       printf("Nothing Added by Scenario\n");
    }
} // End enactScenario()

// Create Frame generator with CE and Scenario parameters
ofdmflexframegen CreateFG(struct CognitiveEngine ce, struct Scenario sc) {

    // Set Modulation Scheme
    // TODO: add other liquid-supported mod schemes
    modulation_scheme ms;
    if (strcmp(ce.modScheme, "QPSK") == 0) {
        ms = LIQUID_MODEM_QPSK;
    }
    else if ( strcmp(ce.modScheme, "BPSK") ==0) {
        ms = LIQUID_MODEM_BPSK;
    }
    else if ( strcmp(ce.modScheme, "OOK") ==0) {
        ms = LIQUID_MODEM_OOK;
    }
    else {
        printf("ERROR: Unkown Modulation Scheme");
        //TODO: Skip current test if given an unkown parameter.
    }

    // Set Cyclic Redundency Check Scheme
    crc_scheme check;
    if (strcmp(ce.crcScheme, "none") == 0) {
        check = LIQUID_CRC_NONE;
        printf("check = LIQUID_CRC_NONE\n");
    }
    else if (strcmp(ce.crcScheme, "checksum") == 0) {
        check = LIQUID_CRC_CHECKSUM;
        printf("check = LIQUID_CRC_CHECKSUM\n");
    }
    else if (strcmp(ce.crcScheme, "8") == 0) {
        check = LIQUID_CRC_8;
        printf("check = LIQUID_CRC_8\n");
    }
    else if (strcmp(ce.crcScheme, "16") == 0) {
        check = LIQUID_CRC_16;
        printf("check = LIQUID_CRC_16\n");
    }
    else if (strcmp(ce.crcScheme, "24") == 0) {
        check = LIQUID_CRC_24;
        printf("check = LIQUID_CRC_24\n");
    }
    else if (strcmp(ce.crcScheme, "32") == 0) {
        check = LIQUID_CRC_32;
        printf("check = LIQUID_CRC_32\n");
    }
    else {
        printf("ERROR: unknown CRC\n");
        //TODO: Skip current test if given an unkown parameter.
    }

    // Set inner forward error correction scheme
    // TODO: add other liquid-supported FEC schemes
    fec_scheme fec0;
    if (strcmp(ce.innerFEC, "none") == 0) {
        fec0 = LIQUID_FEC_NONE;
        printf("fec0 = LIQUID_FEC_NONE\n");
    }
    else if (strcmp(ce.innerFEC, "Hamming74") == 0) {
        fec0 = LIQUID_FEC_HAMMING74;
        printf("fec0 = LIQUID_FEC_HAMMING74\n");
    }
    else if (strcmp(ce.innerFEC, "Hamming128") == 0) {
        fec0 = LIQUID_FEC_HAMMING128;
        printf("fec0 = LIQUID_FEC_HAMMING128\n");
    }
    else if (strcmp(ce.innerFEC, "REP3") == 0) {
        fec0 = LIQUID_FEC_REP3;
        printf("fec0 = LIQUID_FEC_REP3\n");
    }
    else {
        printf("ERROR: unknown inner FEC\n");
        //TODO: Skip current test if given an unkown parameter.
    }

    // Set outer forward error correction scheme
    // TODO: add other liquid-supported FEC schemes
    fec_scheme fec1;
    if (strcmp(ce.outerFEC, "none") == 0) {
        fec1 = LIQUID_FEC_NONE;
        printf("fec1 = LIQUIDFECNONE\n");
    }
    else if (strcmp(ce.outerFEC, "Hamming74") == 0) {
        fec1 = LIQUID_FEC_HAMMING74;
        printf("fec1 = LIQUID_FEC_HAMMING74\n");
    }
    else if (strcmp(ce.outerFEC, "Hamming128") == 0) {
        fec1 = LIQUID_FEC_HAMMING128;
        printf("fec1 = LIQUID_FEC_HAMMING128\n");
    }
    else if (strcmp(ce.outerFEC, "REP3") == 0) {
        fec1 = LIQUID_FEC_REP3;
        printf("fec1 = LIQUID_FEC_REP3\n");
    }
    else {
        printf("ERROR: unknown outer FEC\n");
        //TODO: Skip current test if given an unkown parameter.
    }

    // Frame generation parameters
    ofdmflexframegenprops_s fgprops;

    // Initialize Frame generator and Frame Synchronizer Objects
    ofdmflexframegenprops_init_default(&fgprops);
    fgprops.mod_scheme      = ms;
    fgprops.check           = check;
    fgprops.fec0            = fec0;
    fgprops.fec1            = fec1;
    ofdmflexframegen fg = ofdmflexframegen_create(ce.numSubcarriers, ce.CPLen, ce.taperLen, NULL, &fgprops);

    return fg;
} // End CreateFG()

int rxCallback(unsigned char *  _header,
                int              _header_valid,
                unsigned char *  _payload,
                unsigned int     _payload_len,
                int              _payload_valid,
                framesyncstats_s _stats,
                void *           _userdata)
{
    // Iterator
    int i = 0;

    // Create a client TCP socket
    int socket_to_server = socket(AF_INET, SOCK_STREAM, 0); 
    if( socket_to_server < 0)
    {   
    printf("Receiver Failed to Create Client Socket\n");
    exit(1);
    }   
    printf("Created client socket to server. socket_to_server: %d\n", socket_to_server);

    // Parameters for connecting to server
    // TODO: Allow selection of IP address and port in command line parameters.
    struct sockaddr_in servAddr;
    memset(&servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_port = htons(PORT);
    servAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // Attempt to connect client socket to server
    int connect_status;
    if((connect_status = connect(socket_to_server, (struct sockaddr*)&servAddr, sizeof(servAddr))))
    {   
    printf("Receive Failed to Connect to server.\n");
    printf("connect_status = %d\n", connect_status);
    exit(1);
    }
   
    framesyncstats_print(&_stats); 
      
    // Data that will be sent to server
    // TODO: Send other useful data through feedback array
    float feedback[8];
    feedback[0] = (float) _header_valid;
    feedback[1] = (float) _payload_valid;
    feedback[2] = (float) _stats.evm;
    feedback[3] = (float) _stats.rssi;   
   
    for (i=0; i<8; i++)
    printf("feedback data before transmission: %f\n", feedback[i]);

    // Receiver sends data to server
    //printf("socket_to_server: %d\n", socket_to_server);
    int writeStatus = write(socket_to_server, feedback, 8*sizeof(float));
    printf("Rx writeStatus: %d\n", writeStatus);

    // Receiver closes socket to server
    close(socket_to_server);
    return 0;

} // end rxCallback()

// TODO: Once we are using USRPs, move to an rx.c file that will run independently.
ofdmflexframesync CreateFS(struct CognitiveEngine ce, struct Scenario sc)
{
     ofdmflexframesync fs =
             ofdmflexframesync_create(ce.numSubcarriers, ce.CPLen, ce.taperLen, NULL, rxCallback, NULL);

     return fs;
} // End CreateFS();

// Transmit a packet of data.
// This will need to be modified once we implement the USRPs.
int txGeneratePacket(struct CognitiveEngine ce, ofdmflexframegen * _fg, unsigned char * header, unsigned char * payload)
{
    // Iterator
    int i = 0;
    // Buffers for data
    //unsigned char header[8];
    //unsigned char payload[ce.payloadLen];

    // Generate data
    printf("\n\ngenerating data that will go in frame...\n");
    for (i=0; i<8; i++)
    header[i] = i & 0xff;
    for (i=0; i<ce.payloadLen; i++)
    payload[i] = rand() & 0xff;

    // Assemble frame
    ofdmflexframegen_assemble(*_fg, header, payload, ce.payloadLen);

    return 1;
} // End txGeneratePacket()

int txTransmitPacket(struct CognitiveEngine ce, ofdmflexframegen * _fg, float complex * frameSamples)
{
    int isLastSymbol = ofdmflexframegen_writesymbol(*_fg, frameSamples);
    return isLastSymbol;
} // End txTransmitPacket()

int rxReceivePacket(struct CognitiveEngine ce, ofdmflexframesync * _fs, float complex * frameSamples)
{
    unsigned int symbolLen = ce.numSubcarriers + ce.CPLen;
    ofdmflexframesync_execute(*_fs, frameSamples, symbolLen);
    return 1;
} // End rxReceivePacket()

// Create a TCP socket for the server and bind it to a port
// Then sit and listen/accept all connections and write the data
// to an array that is accessible to the CE
void * startTCPServer(void * _read_buffer )
{
    printf("Server thread called.\n");
    // Iterator
    int i;
    // Buffer for data sent by client. This memory address is also given to CE
    float * read_buffer = (float *) _read_buffer;
    //  Local (server) address
    struct sockaddr_in servAddr;   
    // Parameters of client connection
    struct sockaddr_in clientAddr;              // Client address 
    socklen_t client_addr_size;  // Client address size
    int socket_to_client = -1;
        
    // Create socket for incoming connections 
    int sock_listen;
    if ((sock_listen = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("Transmitter Failed to Create Server Socket.\n");
        exit(1);
    }
    //printf("sock_listen= %d\n", sock_listen);

    // Construct local (server) address structure 
    memset(&servAddr, 0, sizeof(servAddr));       // Zero out structure 
    servAddr.sin_family = AF_INET;                // Internet address family 
    servAddr.sin_addr.s_addr = htonl(INADDR_ANY); // Any incoming interface 
    servAddr.sin_port = htons(PORT);              // Local port 
    // Bind to the local address to a port
    if (bind(sock_listen, (struct sockaddr *) &servAddr, sizeof(servAddr)) < 0)
    {
        printf("bind() error\n");
        exit(1);
    }

    // Listen and accept connections indefinitely
    while (1)
    {
        // listen for connections on socket
        if (listen(sock_listen, MAXPENDING) < 0)
        {
            printf("Failed to Set Sleeping (listening) Mode\n");
            exit(1);
        }
        printf("Server is now in listening mode\n");

        // Accept a connection from client
        //printf("Server is waiting to accept connection from client\n");
        socket_to_client = accept(sock_listen, (struct sockaddr *)&clientAddr, &client_addr_size);
        //printf("socket_to_client= %d\n", socket_to_client);
        if(socket_to_client< 0)
        {
            printf("Sever Failed to Connect to Client\n");
            exit(1);
        }
        printf("Server has accepted connection from client\n");
        // Transmitter receives data from client (receiver)
            // Zero the read buffer. Then read the data into it.
            bzero(read_buffer, 256);
            int read_status = -1;   // indicates success/failure of read operation.
            read_status = read(socket_to_client, read_buffer, 255);
        // Print the data received
        //printf("read_status= %d\n", read_status);
        //printf("\nServer (transmitter) received:\n" );
        //printf("readbuffer[0]= %f\n", read_buffer[0]);
        //printf("readbuffer[1]= %f\n", read_buffer[1]);
        //printf("readbuffer[2]= %f\n", read_buffer[2]);
    } // End listening While loop
} // End startTCPServer()

int ceAnalyzeData(struct CognitiveEngine * ce, float * feedback)
{
    int i = 0;
    printf("In ceAnalyzeData():\nfeedback=\n");
    for (i = 0; i<4;i++) {
        printf("feedback[%d]= %f\n", i, feedback[i]);
    }
    // Copy the data from the server
    //feedback[100];
    if (strcmp(ce->goal, "payload_valid") == 0)
    {
        printf("Goal is payload_valid. Setting latestGoalValue to %f\n", feedback[1]);
        ce->latestGoalValue = feedback[1];
    }
    // TODO: implement if statements for other possible goals.

    return 1;
} // End ceAnalyzeData()

int ceOptimized(struct CognitiveEngine ce)
{
   printf("Checking if goal value has been reached.\n");
   if (ce.latestGoalValue >= ce.threshold)
   {
       printf("Goal is reached\n");
       return 1;
   }
   printf("Goal not reached yet.\n");
   return 0;
} // end ceOptimized()

int ceModifyTxParams(struct CognitiveEngine * ce, float * feedback)
{
    printf("Modifying Tx parameters");
    // TODO: Implement a similar if statement for each possible option
    // that can be adapted.
    if (strcmp(ce->option_to_adapt, "mod_scheme") == 0) {
        strcpy(ce->modScheme, "BPSK");
    }
    return 1;
}   // End ceModifyTxParams()

int main()
{
    // Threading parameters (to open Server in its own thread)
    pthread_t TCPServerThread;   // Pointer to thread ID
    int serverThreadReturn = 0;  // return value of creating TCPServer thread

    // Array that will be accessible to both Server and CE.
    // Server uses it to pass data to CE.
    float feedback[100];

    // Number of Cognitive Engines
    int NumCE = 1;
    int NumSc = 2;

    // Iterators
    int i_CE = 0;
    int i_Sc = 0;
    int DoneTransmitting = 0;
    int N = 0;                 // Iterations of transmission before receiving.
    int i_N = 0;
    int isLastSymbol = 0;
    char scenario_list [30][60];
    printf ("\nDeclared scenario array");
    char cogengine_list [30][60];
    
    printf("variables declared.\n");

    readCEMasterFile(cogengine_list);  
    readScMasterFile(scenario_list);  
    printf ("\nCalled readScMasterFile function\n");

    // Cognitive engine struct used in each test
    struct CognitiveEngine ce = CreateCognitiveEngine();
    // Scenario struct used in each test
    struct Scenario sc = CreateScenario();

    printf("structs declared\n");
    // framegenerator object used in each test
    ofdmflexframegen fg;

    // framesynchronizer object used in each test
    // TODO: Once we are using USRPs, move to an rx.c file that will run independently.
    ofdmflexframesync fs;

    printf("frame objects declared\n");

    // Buffers for packet/frame data
    unsigned char header[8];                // Must always be 8 bytes for ofdmflexframe
    unsigned char payload[1000];            // Large enough to accomodate any (reasonable) payload that
                                            // the CE wants to use.
    float complex frameSamples[10000];      // Buffer of frame samples for each symbol.
                                            // Large enough to accomodate any (reasonable) payload that 
                                            // the CE wants to use.

    ////////////////////// End variable initializations.

    // Begin TCP Server Thread
    serverThreadReturn = pthread_create( &TCPServerThread, NULL, startTCPServer, (void*) feedback);
    // Allow server time to finish initialization
    sleep(.1);

    // Begin running tests

    // For each Cognitive Engine
    for (i_CE=0; i_CE<NumCE; i_CE++)
    {
        printf("\nStarting Cognitive Engine %d\n", i_CE +1);
        // Initialize current CE
        ce = CreateCognitiveEngine();
        readCEConfigFile(&ce,cogengine_list[i_CE]);
        
        // Run each CE through each scenario
        for (i_Sc= 0; i_Sc<NumSc; i_Sc++)
        {
            printf("Starting Scenario %d\n", i_Sc +1);
            // Initialize current Scenario
            sc = CreateScenario();
            // TODO: Implement reading from config files
            printf("Before Calling Config_Scenario\n");
            printf("scenario_list[i_Sc]=%s\n", scenario_list[i_Sc]);
            readScConfigFile(&sc,scenario_list[i_Sc]);
            printf ("After Calling Config_Scenario\n");

            // Initialize Transmitter Defaults for current CE and Sc
            fg = CreateFG(ce, sc);  // Create ofdmflexframegen object with given parameters
                //TODO: Initialize Connection to USRP                                     

            // Initialize Receiver Defaults for current CE and Sc
            // TODO: Once we are using USRPs, move to an rx.c file that will run independently.
            fs = CreateFS(ce, sc);

            // Begin Testing Scenario
            DoneTransmitting = 0;
            while(!DoneTransmitting)
            {
                printf("DoneTransmitting= %d\n", DoneTransmitting);
                // Generate data to go into frame (packet)
                txGeneratePacket(ce, &fg, header, payload);
                // Simulate N tranmissions before simulating receiving them.
                // i.e. Need to transmit each symbol in frame.
                isLastSymbol = 0;
                //N = ce.iterations;
                N = 0;
                //for (i_N= 0; i_N< N; i_N++)
                while (!isLastSymbol) 
                {
                    // TODO: Create this function
                    isLastSymbol = txTransmitPacket(ce, &fg, frameSamples);

                    // TODO: Create this function
                    //enactScenario();

                    // TODO: Create this function
                    // Store a copy of the packet that was transmitted. For reference.
                    // txStoreTransmittedPacket();
                
                    // TODO: Once we are using USRPs, move to an rx.c file that will run independently.
                    // Rx Receives packet
                    rxReceivePacket(ce, &fs, frameSamples);
                } // End Transmition For loop

                // Receive and analyze data from rx
                ceAnalyzeData(&ce, feedback);
                // Modify transmission parameters (in fg and in USRP) accordingly
                if (!ceOptimized(ce)) 
                {
                    printf("ceOptimized() returned false\n");
                    ceModifyTxParams(&ce, feedback);
                }
                else
                {
                    printf("ceOptimized() returned true\n");
                    DoneTransmitting = 1;
                    printf("else: DoneTransmitting= %d\n", DoneTransmitting);
                }

            } // End Test While loop
            
        } // End Scenario For loop

    } // End CE for loop

    return 0;
}








