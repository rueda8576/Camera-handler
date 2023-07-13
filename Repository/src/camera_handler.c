#include "../include/camera_handler.h"

char* Get_Telecomand()
{
    //Socket Parameters
    int                 server_socket;
    int                 socket_buffer_size                  = UDP_PACKET_SIZE;
    struct sockaddr_in  server_address;
    struct sockaddr_in  client_address;
    socklen_t           client_address_len                  = sizeof(client_address);
    
    int                 bytes_recv;
    char*               telemetry                           = (char*)(malloc(MAX_BUFFER_SIZE_OPT));
    

    //Destinatary IP for frames
    const char*         client_ip                           = "192.168.10.33";
    int                 client_port                         = CLIENT_PORT;
    int                 server_port                         = SERVER_PORT;

    // Set client address
    client_address.sin_addr.s_addr  = inet_addr(client_ip);                     // Server IP (Raspberry Pi)
    client_address.sin_family       = PF_INET;
    client_address.sin_port         = htons(client_port);

    server_address.sin_family       = PF_INET;
    server_address.sin_addr.s_addr  = htonl(INADDR_ANY);
    server_address.sin_port         = htons( server_port );

    memset(telemetry, 0, MAX_BUFFER_SIZE_OPT);

    // Create server socket
    if ((server_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, (char *) &socket_buffer_size, sizeof(socket_buffer_size)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind server socket to port and listen any address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) != 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Server running and listening on port %d...\n", CLIENT_PORT);

    //Get client opt
    bytes_recv = recvfrom(server_socket, telemetry, sizeof(telemetry), 0, (struct sockaddr *)&client_address, &client_address_len);
    if (bytes_recv == -1)
    {
        perror("Error getting client option\n");
        exit(EXIT_FAILURE);
    }

    printf("\nReceived option: %s from client %s:%d\n", telemetry, inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

    //Close socket
    if (close(server_socket) < 0)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }

    return telemetry;
}

VmbError_t GetFeatures()
{
    //Socket Parameters
    int                 server_socket;
    int                 socket_buffer_size      = UDP_PACKET_SIZE;
    struct sockaddr_in  server_address;
    struct sockaddr_in  client_address;

    //Destinatary IP for frames
    //"192.168.1.50";
    const char*         client_ip               = "192.168.10.33";
    int                 client_port             = CLIENT_PORT;
    int                 server_port             = SERVER_PORT;

    // Set client address
    client_address.sin_addr.s_addr              = inet_addr(client_ip);                 // Server IP (Raspberry Pi)
    client_address.sin_family                   = PF_INET;
    client_address.sin_port                     = htons(client_port);
    server_address.sin_family                   = PF_INET;
    server_address.sin_addr.s_addr              = htonl(INADDR_ANY);
    server_address.sin_port                     = htons( server_port );

    // Create server socket
    if ((server_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, (char *) &socket_buffer_size, sizeof(socket_buffer_size)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind server socket to port and listen any address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) != 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //Vimba Parameters
    VmbError_t          error                   = VmbErrorSuccess;
    VmbHandle_t         cameraHandle            = NULL;
    VmbUint32_t         count                   = 0;
    VmbAccessMode_t     cameraAccessMode        = VmbAccessModeFull;                    // We open the camera with full access
    VmbFeatureInfo_t*   pFeatures               = NULL;                                 // A list of features
    char*               pStrValue               = NULL;                                 // A string value
    VmbBool_t           bValue                  = VmbBoolFalse;                         // A bool value
    VmbInt64_t          nValue                  = 0;                                    // An int value
    double              fValue                  = 0.0;                                  // A float value
    VmbUint32_t         i                       = 0;

    // Initialize Vimba
    error = VmbStartup();
    if (error != VmbErrorSuccess)
    {
        printf( "Error initializing Vimba: %d\n", error );
        return error;
    }

    //Ping GigE camera
    error = VmbFeatureCommandRun( gVimbaHandle, "GeVDiscoveryAllOnce" );
    if( error != VmbErrorSuccess )
    {
        printf( "Could not ping GigE cameras over the network. Reason: %d\n", error );
        return error;
    }
    
    // Open camera
    error = VmbCameraOpen("DEV_000F315DF05B", cameraAccessMode, &cameraHandle);
    if ( error != VmbErrorSuccess )
    {
        printf("Error opening camera: %d\n", error);
        VmbShutdown();
        return error;
    }

    // Get the amount of features
    error = VmbFeaturesList( cameraHandle, NULL, 0, &count, sizeof *pFeatures );                             
    if ( VmbErrorSuccess != error && 0 >= count )
    {
        printf( "Could not get features or the ancillary data does not provide any. Error code: %d\n", error );
        VmbShutdown();
        return error;
    }

    //Allocate memory for Features of the camera
    pFeatures = (VmbFeatureInfo_t *)malloc( count * sizeof *pFeatures );
    if( pFeatures == NULL )
    {
        printf( "Could not allocate feature list.\n" );
        VmbShutdown();
        return error;
    }

    // Get the features
    error = VmbFeaturesList( cameraHandle, pFeatures, count, &count, sizeof *pFeatures );                    
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not get features. Error code: %d\n", error );
        VmbShutdown();
        return error;
    }

    //Send amount of features to client
    char s_count[10] = {0};
    sprintf(s_count, "%d", count);
    if (sendto(server_socket, s_count, strlen(s_count), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
    {
        printf("s_count sending failed\n");
        exit(EXIT_FAILURE);
    }

    //Send features to client (Name, description and value)
    for ( i = 0; i < count; ++i )
    {
        char s_FeatureName[1024]            = {0};
        char s_FeatureDescription[1024]     = {0};
        char s_FeatureValue[1024]           = {0};
        sprintf(s_FeatureName           , "%s"  , pFeatures[i].name);
        sprintf(s_FeatureDescription    , "%s"  , pFeatures[i].description);

        switch ( pFeatures[i].featureDataType )
        {
            case VmbFeatureDataBool:
                error = VmbFeatureBoolGet( cameraHandle, pFeatures[i].name, &bValue );
                if ( VmbErrorSuccess == error )
                {
                    sprintf(s_FeatureValue, "%d", bValue);
                }
                break;
            case VmbFeatureDataEnum:
                error = VmbFeatureEnumGet( cameraHandle, pFeatures[i].name, (const char**)&pStrValue );
                if ( VmbErrorSuccess == error )
                {
                    strcpy(s_FeatureValue, pStrValue);
                }
                break;
            case VmbFeatureDataFloat:
                error = VmbFeatureFloatGet( cameraHandle, pFeatures[i].name, &fValue );
                {
                    sprintf(s_FeatureValue, "%f", fValue);
                }
                break;
            case VmbFeatureDataInt:
                error = VmbFeatureIntGet( cameraHandle, pFeatures[i].name, &nValue );
                {
                    sprintf(s_FeatureValue, "%lld", nValue);
                }
                break;
            case VmbFeatureDataString:
                {
                VmbUint32_t nSize = 0;
                error = VmbFeatureStringGet( cameraHandle, pFeatures[i].name, NULL, 0, &nSize );
                if ( VmbErrorSuccess == error && 0 < nSize )
                {
                    pStrValue = (char*)malloc( sizeof *pStrValue * nSize );
                    if ( NULL != pStrValue )
                    {
                        error = VmbFeatureStringGet( cameraHandle, pFeatures[i].name, pStrValue, nSize, &nSize );
                        if ( VmbErrorSuccess == error )
                        {
                            strcpy(s_FeatureValue, pStrValue);
                        }
                        free( pStrValue );
                        pStrValue = NULL;
                    }
                    else
                    {
                        printf( "Could not allocate string.\n" );
                    }
                }
                }
                break;
            case VmbFeatureDataCommand:
            default:
                error = 1;
                break;
        }
        
        //Send name
        if (sendto(server_socket, s_FeatureName, strlen(s_FeatureName), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
        {
            printf("s_FeatureName sending failed\n");
            exit(EXIT_FAILURE);
        }

        //Send Decription
        if (sendto(server_socket, s_FeatureDescription, strlen(s_FeatureDescription), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
        {
            printf("s_FeatureName sending failed\n");
            exit(EXIT_FAILURE);
        }
        
        //Send value
        if (sendto(server_socket, s_FeatureValue, strlen(s_FeatureValue), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
        {
            printf("s_FeatureValue sending failed\n");
            exit(EXIT_FAILURE);
        }

    }

    free( pFeatures );
    pFeatures = NULL;

    //Close socket
    if (close(server_socket) < 0)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }

    // Cleanup
    VmbCameraClose(cameraHandle);
    VmbShutdown();

    return error;
}

VmbError_t ImageAcquisition()
{
    //Socket Parameters
    int                 server_socket;
    int                 socket_buffer_size      = UDP_PACKET_SIZE;

    struct sockaddr_in  server_address;
    struct sockaddr_in  client_address;
    //struct sockaddr_in  processor_address;

    socklen_t   client_address_len      = sizeof(client_address);
    int         num_packets             = 0;
    int         total_bytes_sent        = 0;
    int         bytes_sent;
    int         bytes_recv;

    int         server_port             = SERVER_PORT;

    //Destinatary IP for frames
    const char*         client_ip               = "192.168.10.33";
    int                 client_port             = CLIENT_PORT;
    
    //const char*         processor_ip            = "192.168.1.11";
    //int                 processor_port          = PROCESSOR_PORT;

    // Set addresses
    client_address.sin_addr.s_addr              = inet_addr(client_ip);             // Server IP (Raspberry Pi)
    client_address.sin_family                   = PF_INET;
    client_address.sin_port                     = htons(client_port);

    server_address.sin_family                   = PF_INET;
    server_address.sin_addr.s_addr              = htonl(INADDR_ANY);
    server_address.sin_port                     = htons( server_port );

    /* processor_address.sin_addr.s_addr           = inet_addr(processor_ip);          // Processor board IP
    processor_address.sin_family                = PF_INET;
    processor_address.sin_port                  = htons(processor_port); */
 
    //Vimba Parameters
    VmbError_t          error                       = VmbErrorSuccess;
    VmbHandle_t         cameraHandle                = NULL;
    VmbFrame_t*         frames_AUTO [MIN_FRAMES];
    VmbFrame_t*         frames_HDR  [HDR_FRAMES];
    VmbFrame_t*         frames_GOOD [GOOD_FRAMES];
    VmbAccessMode_t     cameraAccessMode            = VmbAccessModeFull;    // We open the camera with full access
    VmbBool_t           bIsCommandDone              = VmbBoolFalse;         // Has a command finished execution
    VmbInt64_t          nPayloadSize                = 0;                    // The size of one frame

    // Initialize Vimba
    error = VmbStartup();
    if (error != VmbErrorSuccess)
    {
        printf( "Error initializing Vimba: %d\n", error );
        return 1;
    }

    //Ping GigE camera
    error = VmbFeatureCommandRun( gVimbaHandle, "GeVDiscoveryAllOnce" );
    if( error != VmbErrorSuccess )
    {
        printf( "Could not ping GigE cameras over the network. Reason: %d\n", error );
    }
    
    // Open camera
    error = VmbCameraOpen("DEV_000F315DF05B", cameraAccessMode, &cameraHandle);
    if (error != VmbErrorSuccess)
    {
        printf("Error opening camera: %d\n", error);
        VmbShutdown();
        return 1;
    }

    //Set multicast mode
    error = VmbFeatureBoolSet(cameraHandle, "MulticastEnable", false);
    if (error != VmbErrorSuccess)
    {
        printf("Error setting multicast mode: %d\n", error);
        return 1;
    }
    
    // Set the GeV packet size to the highest possible value
    if ( VmbErrorSuccess == VmbFeatureCommandRun(cameraHandle, "GVSPAdjustPacketSize"))
    {
        do
        {
            if ( VmbErrorSuccess != VmbFeatureCommandIsDone(cameraHandle, "GVSPAdjustPacketSize", &bIsCommandDone))
            {
                break;
            }
        } while ( VmbBoolFalse == bIsCommandDone );
    }
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not adjust packet size. Error code: %d\n", error );
        VmbShutdown();
        return 1;
    }
    
    // Set pixel format
    error = VmbFeatureEnumSet( cameraHandle, "PixelFormat", PIXEL_FORMAT );
    
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not set pixel format. Error code: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //Set Balance White Config
    error = VmbFeatureEnumSet(cameraHandle, "BalanceWhiteAuto", "Off");
    if (error != VmbErrorSuccess)
    {
        printf("Error setting exposure mode: %d\n", error);
        return 1;
    }

    error = VmbFeatureEnumSet(cameraHandle, "BalanceRatioSelector", "Red");
    if (error != VmbErrorSuccess)
    {
        printf("Error setting exposure mode: %d\n", error);
        return 1;
    }

    if ( VmbErrorSuccess != VmbFeatureFloatSet( cameraHandle , "BalanceRatioAbs", GAIN_RED ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    error = VmbFeatureEnumSet(cameraHandle, "BalanceRatioSelector", "Blue");
    if (error != VmbErrorSuccess)
    {
        printf("Error setting exposure mode: %d\n", error);
        return 1;
    }

    if ( VmbErrorSuccess != VmbFeatureFloatSet( cameraHandle , "BalanceRatioAbs", GAIN_BLUE ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //set Acquisition mode
    error = VmbFeatureEnumSet( cameraHandle , "AcquisitionMode", "MultiFrame" );
    if (error != VmbErrorSuccess)
    {
        printf("Error setting Acquisition mode to Multiframe: %d\n", error);
        return 1;
    }

    //set Frame Count
    error = VmbFeatureIntSet( cameraHandle , "AcquisitionFrameCount", NUM_FRAMES );
    if (error != VmbErrorSuccess)
    {
        printf("Error setting Frame Count: %d\n", error);
        return 1;
    }
    
    //Frame dimensions
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "Height", HEIGHT ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "Width", WIDTH ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //OffsetX
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetX", OFFSET_X ))
    {
        printf( "Error setting OffsetX: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //OffsetX
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetY", OFFSET_Y ))
    {
        printf( "Error setting OffsetY: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Bottom
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionBottom", DSP_SUBREGION_BOTTOM ))
    {
        printf( "Error setting DSP_SUBREGION_BOTTOM: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Left
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionLeft", DSP_SUBREGION_LEFT ))
    {
        printf( "Error setting DSP_SUBREGION_LEFT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Top
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionTop", DSP_SUBREGION_TOP ))
    {
        printf( "Error setting DSP_SUBREGION_TOP: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Right
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionRight", DSP_SUBREGION_RIGHT ))
    {
        printf( "Error setting DSP_SUBREGION_RIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //Exposure Auto Max (1 seg)
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "ExposureAutoMax", 1000000 ))
    {
        printf( "Error setting EXPOSURE_AUTO_MAX: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //StreamBytesPerSecond
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "StreamBytesPerSecond", STREAM_BYTES_PER_SECOND ))
    {
        printf( "Error setting StreamBytesPerSecond: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //Configure HDR
    //1. Configurar Exposure Auto y Timed, Gain NO Auto y valor nulo
    if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureAuto", "Continuous" ) )
    {
        printf( "Error setting ExposureAuto: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureMode", "Timed" ) )
    {
        printf( "Error setting ExposureMode: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "GainAuto", "Off" ) )
    {
        printf( "Error setting GainAuto: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureFloatSet( cameraHandle, "Gain", 0 ) )
    {
        printf( "Error setting Gain: %d\n", error );
        VmbShutdown();
        return 1;
    }
    
    // Evaluate frame size
    error = VmbFeatureIntGet( cameraHandle, "PayloadSize", &nPayloadSize );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error evaluating frames size: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //Calcular ExposureTime_AUTO

    //Anunciar y alojar memoria para frames
    for (int i = 0; i < MIN_FRAMES; i++)
    {
        frames_AUTO[i]               = (VmbFrame_t*)malloc(sizeof(VmbFrame_t));
        frames_AUTO[i] -> buffer     = (unsigned char*)malloc( (VmbUint32_t)nPayloadSize );
        frames_AUTO[i] -> bufferSize = (VmbUint32_t)nPayloadSize;

        // Announce Frame
        error = VmbFrameAnnounce(cameraHandle, frames_AUTO[i], (VmbUint32_t)sizeof( VmbFrame_t ));
        if (error != VmbErrorSuccess) {
            printf("Error announcing frames_AUTO: %d\n", error);
            VmbCaptureEnd(cameraHandle);
            VmbCameraClose(cameraHandle);
            VmbShutdown();
            return 1;
        }
    }

    double  ExposureTime_AUTO[MIN_FRAMES]   = {60};
    double  Gain                            = 0;
    int     i                               = 2;

    // Start Capture Engine
    error = VmbCaptureStart(cameraHandle);
    if (error != VmbErrorSuccess)
    {
        printf("Error starting capture: %d\n", error);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    //Queue 1er frame
    error = VmbCaptureFrameQueue(cameraHandle, frames_AUTO[0], NULL);
    if (error != VmbErrorSuccess)
    {
        printf("Error queueing first frames_AUTO: %d\n", error);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    // Start Acquisition
    error = VmbFeatureCommandRun( cameraHandle,"AcquisitionStart" );
    if (error != VmbErrorSuccess) {
        printf("Error startig acquisition: %d\n", error);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    while (true)
    {
        error = VmbCaptureFrameWait(cameraHandle, frames_AUTO[0], TIMEOUT);
        if (error == VmbErrorSuccess)
        {
            break;
        }
    }
    error = VmbFeatureFloatGet( cameraHandle, "ExposureTimeAbs", &ExposureTime_AUTO[0] );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error getting ExposureTimeAbs value: %d\n", error );
        VmbShutdown();
        return 1;
    }
    error = VmbCaptureFrameQueue(cameraHandle, frames_AUTO[1], NULL);
    if (error != VmbErrorSuccess)
    {
        printf("Error queueing first frames_AUTO: %d\n", error);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    while (true)
    {
        error = VmbCaptureFrameWait(cameraHandle, frames_AUTO[1], TIMEOUT);
        if (error == VmbErrorSuccess)
        {
            break;
        }
    }
    error = VmbFeatureFloatGet( cameraHandle, "ExposureTimeAbs", &ExposureTime_AUTO[1] );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error getting ExposureTimeAbs value: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //2. Obtener N frames para estabilizar ExposureTimeAbs
    for (; i < MIN_FRAMES; i++)
    {     
        error = VmbCaptureFrameQueue(cameraHandle, frames_AUTO[i], NULL);
        if (error != VmbErrorSuccess)
        {
            printf("Error queueing frames_AUTO: %d\n", error);
            VmbCaptureEnd(cameraHandle);
            VmbCameraClose(cameraHandle);
            VmbShutdown();
            return 1;
        }

        while (true)
        {
            error = VmbCaptureFrameWait(cameraHandle, frames_AUTO[i], TIMEOUT);
            if (error == VmbErrorSuccess)
            {
                break;
            }
        }

        error = VmbFeatureFloatGet( cameraHandle, "ExposureTimeAbs", &ExposureTime_AUTO[i] );
        if ( VmbErrorSuccess != error )
        {
            printf( "Error getting ExposureTime_AUTO value: %d\n", error );
            VmbShutdown();
            return 1;
        }

        if (ExposureTime_AUTO[i] == ExposureTime_AUTO[i-1] && ExposureTime_AUTO[i] == ExposureTime_AUTO[i-2])
        {
            break;
        }

        if ((i == (MIN_FRAMES - 1) && ExposureTime_AUTO[i] != ExposureTime_AUTO[i-1]))
        {
            printf("AutoExposureAbs not estabilized...\n");
        }
        printf("ExposureTime_AUTO = %f\n", ExposureTime_AUTO[i]);
    }

    // Stop Acquisition
    error = VmbFeatureCommandRun( cameraHandle,"AcquisitionStop" );
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not stop acquisition. Error code: %d\n", error );
        //VmbFrameRevoke(cameraHandle, frame);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    // Stop Capture Engine
    error = VmbCaptureEnd(cameraHandle);
    if (error != VmbErrorSuccess)
    {
        printf("Error stopping capture: %d\n", error);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    // Flush the capture queue
    error = VmbCaptureQueueFlush( cameraHandle );
    if (error != VmbErrorSuccess)
    {
        printf("Error flushing capture queue: %d\n", error);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }

    //4. Configurar AutoExposure Off
    if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureAuto", "Off" ) )
    {
        printf( "Error setting ExposureAuto: %d\n", error );
        VmbShutdown();
        return 1;
    }

    if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureMode", "PieceWiseLinearHDR" ) )
    {
        printf( "Error setting ExposureMode to PieceWiseLinearHDR: %d\n", error );
        VmbShutdown();
        return 1;
    }

    // Free memory for the frames
    for (int i = 0; i < MIN_FRAMES; i++)
    {
        if (NULL != frames_AUTO[i] -> buffer)
        {
            error = VmbFrameRevoke(cameraHandle, frames_AUTO[i]);
            if (error != VmbErrorSuccess)
            {
                printf("Error: Failed to revoke frames_AUTO\n");
            }
            free(frames_AUTO[i] -> buffer);
            frames_AUTO[i] -> buffer = NULL;
            free(frames_AUTO[i]);
        }
    }

    //Parámetros HDR
    double      HDR_ExposureTimeAbs =   1.2 * ExposureTime_AUTO[i-1];
    double      ExposureTimePWL1    =   0.1 * HDR_ExposureTimeAbs;
    double      ExposureTimePWL2    =   0.5 * HDR_ExposureTimeAbs;
    
    int         ThresholdPWL1               = THRESHOLD_1;
    int         ThresholdPWL2               = THRESHOLD_2;

    //5. Configurar ExposureTimeAbs para HDR
    error = VmbFeatureFloatSet( cameraHandle, "ExposureTimeAbs", HDR_ExposureTimeAbs );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error setting ExposureTimeAbs value: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //6. Configurar valores HDR
    error = VmbFeatureFloatSet( cameraHandle, "ExposureTimePWL1", ExposureTimePWL1 );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error setting ExposureTimePWL1 value: %d\n", error );
        VmbShutdown();
        return 1;
    }
    error = VmbFeatureFloatSet( cameraHandle, "ExposureTimePWL2", ExposureTimePWL2 );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error setting ExposureTimePWL2 value: %d\n", error );
        VmbShutdown();
        return 1;
    }
    error = VmbFeatureIntSet( cameraHandle, "ThresholdPWL1", ThresholdPWL1 );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error setting ThresholdPWL1 value: %d\n", error );
        VmbShutdown();
        return 1;
    }
    error = VmbFeatureIntSet( cameraHandle, "ThresholdPWL2", ThresholdPWL2 );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error setting ThresholdPWL2 value: %d\n", error );
        VmbShutdown();
        return 1;
    }

    for (int i = 0; i < HDR_FRAMES; i++)
    {
        frames_HDR[i]               = (VmbFrame_t*)malloc(sizeof(VmbFrame_t));
        frames_HDR[i] -> buffer     = (unsigned char*)malloc( (VmbUint32_t)nPayloadSize );
        frames_HDR[i] -> bufferSize = (VmbUint32_t)nPayloadSize;

        // Announce Frame
        error = VmbFrameAnnounce(cameraHandle, frames_HDR[i], (VmbUint32_t)sizeof( VmbFrame_t ));
        if (error != VmbErrorSuccess) {
            printf("Error announcing frames_HDR: %d\n", error);
            VmbCaptureEnd(cameraHandle);
            VmbCameraClose(cameraHandle);
            VmbShutdown();
            return 1;
        }
    }
    for (int i = 0; i < GOOD_FRAMES; i++)
    {
        frames_GOOD[i]               = (VmbFrame_t*)malloc(sizeof(VmbFrame_t));
        frames_GOOD[i] -> buffer     = (unsigned char*)malloc( (VmbUint32_t)nPayloadSize );
        frames_GOOD[i] -> bufferSize = (VmbUint32_t)nPayloadSize;

        // Announce Frame
        error = VmbFrameAnnounce(cameraHandle, frames_GOOD[i], (VmbUint32_t)sizeof( VmbFrame_t ));
        if (error != VmbErrorSuccess) {
            printf("Error announcing frames_GOOD: %d\n", error);
            VmbCaptureEnd(cameraHandle);
            VmbCameraClose(cameraHandle);
            VmbShutdown();
            return 1;
        }
    }
    //Correr 3 frames para estabilizar el frame HDR                                   
    // Start Capture Engine
    error = VmbCaptureStart(cameraHandle);
    if (error != VmbErrorSuccess)
    {
        printf("Error starting capture: %d\n", error);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    //Queue 1er frame
    error = VmbCaptureFrameQueue(cameraHandle, frames_HDR[0], NULL);
    if (error != VmbErrorSuccess)
    {
        printf("Error queueing first frames_AUTO: %d\n", error);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }  
    // Start Acquisition
    error = VmbFeatureCommandRun( cameraHandle,"AcquisitionStart" );
    if (error != VmbErrorSuccess) {
        printf("Error startig acquisition: %d\n", error);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    while (true)
    {
        error = VmbCaptureFrameWait(cameraHandle, frames_HDR[0], TIMEOUT);
        if (error == VmbErrorSuccess)
        {
            break;
        }
    }
    int j = 1;
    
    for (; j < HDR_FRAMES; j++)
    {
        //Siguiente frame
        error = VmbCaptureFrameQueue(cameraHandle, frames_HDR[j], NULL);
        if (error != VmbErrorSuccess)
        {
            printf("Error queueing frames_HDR: %d\n", error);
            VmbCaptureEnd(cameraHandle);
            VmbCameraClose(cameraHandle);
            VmbShutdown();
            return 1;
        }
        
        while (true)
        {
            error = VmbCaptureFrameWait(cameraHandle, frames_HDR[j], TIMEOUT);
            if (error == VmbErrorSuccess)
            {
                break;
            }
        }
    }
    int num = 0;
    //TOMAR FRAME NO CORRUPTO
    for (; num < GOOD_FRAMES; num++)
    {
        error = VmbCaptureFrameQueue(cameraHandle, frames_GOOD[num], NULL);
        if (error != VmbErrorSuccess)
        {
            printf("Error queueing frames_GOOD: %d\n", error);
            VmbCaptureEnd(cameraHandle);
            VmbCameraClose(cameraHandle);
            VmbShutdown();
            return 1;
        }
        while (true)
        {
            error = VmbCaptureFrameWait(cameraHandle, frames_GOOD[num], TIMEOUT);
            if (error == VmbErrorSuccess)
            {
                break;
            }
        }                
        if (VmbFrameStatusComplete == frames_GOOD[num] -> receiveStatus) {  break; }
        else 
        {
            printf("frame %d corrupted\n", num);
        }
        
    }
    // Stop Acquisition
    error = VmbFeatureCommandRun( cameraHandle,"AcquisitionStop" );
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not stop acquisition. Error code: %d\n", error );
        //VmbFrameRevoke(cameraHandle, frame);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    // Stop Capture Engine
    error = VmbCaptureEnd(cameraHandle);
    if (error != VmbErrorSuccess)
    {
        printf("Error stopping capture: %d\n", error);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    // Flush the capture queue
    error = VmbCaptureQueueFlush( cameraHandle );
    if (error != VmbErrorSuccess)
    {
        printf("Error flushing capture queue: %d\n", error);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }

    //Get Gain value
    error = VmbFeatureFloatGet( cameraHandle, "Gain", &Gain );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error getting Gain value: %d\n", error );
        VmbShutdown();
        return 1;
    }
    
    //Send frame

    // Create server socket
    if ((server_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, (char *) &socket_buffer_size, sizeof(socket_buffer_size)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind server socket to port and listen any address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) != 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Send image data over socket
    char        s_ExposureTimeAbs[9]  = {0};
    char        s_Gain[9]             = {0};

    sprintf(s_ExposureTimeAbs   , "%f", ExposureTime_AUTO[i-1]);
    sprintf(s_Gain              , "%f", Gain);

    printf("\nSending frame...\n");

    //ExposureTimeAbs
    if (sendto(server_socket, s_ExposureTimeAbs, strlen(s_ExposureTimeAbs), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
    {
        printf("s_height sending failed\n");
        exit(EXIT_FAILURE);
    }
    
    //Gain
    if (sendto(server_socket, s_Gain, strlen(s_Gain), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
    {
        printf("s_Gain sending failed\n");
        exit(EXIT_FAILURE);
    }

    //Send frame.buffer (Split buffer for sending purpose)
    char* aux_buffer = (char*)malloc(UDP_PACKET_SIZE * sizeof(char));
    for (int k = 0; k < (frames_GOOD[num] -> bufferSize); k += UDP_PACKET_SIZE)
    {
        bytes_sent = 0;
        memcpy(aux_buffer, ((char*)frames_GOOD[num] -> buffer) + k, UDP_PACKET_SIZE);
        bytes_sent = sendto(server_socket, aux_buffer, UDP_PACKET_SIZE, 0, (struct sockaddr *)&client_address, sizeof(client_address));
        if (bytes_sent > 0)
        {
            total_bytes_sent = bytes_sent + total_bytes_sent;
            num_packets++;
            //printf("total_bytes_sent = %d\n", total_bytes_sent);

            //Wait for message from client to send next frame
            bytes_recv = 0;
            char confirmation[MAX_BUFFER_SIZE_OPT] = {0};
            bytes_recv = recvfrom(server_socket, confirmation, sizeof(confirmation), 0, (struct sockaddr *)&client_address, &client_address_len);
            if (bytes_recv == -1)
            {
                perror("Error getting message from client to stop acquisition\n");
                VmbCaptureEnd(cameraHandle);
                VmbCameraClose(cameraHandle);  
                exit(EXIT_FAILURE);
            }
        }
        if (bytes_sent == 0)
        {
            free(aux_buffer);
            break;
        }
    }
    free(aux_buffer);

    //Close socket
    if (close(server_socket) < 0)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }

    // Flush the capture queue
    VmbCaptureQueueFlush( cameraHandle );

    // Free memory for the frames
    for (int i = 0; i < HDR_FRAMES; i++)
    {
        if (NULL != frames_HDR[i] -> buffer)
        {
            error = VmbFrameRevoke(cameraHandle, frames_HDR[i]);
            if (error != VmbErrorSuccess)
            {
                printf("Error: Failed to revoke frames_HDR\n");
            }
            free(frames_HDR[i] -> buffer);
            frames_HDR[i] -> buffer = NULL;
            free(frames_HDR[i]);
        }
    }
    for (int i = 0; i < GOOD_FRAMES; i++)
    {
        if (NULL != frames_GOOD[i] -> buffer)
        {
            error = VmbFrameRevoke(cameraHandle, frames_GOOD[i]);
            if (error != VmbErrorSuccess)
            {
                printf("Error: Failed to revoke frames_GOOD\n");
            }
            free(frames_GOOD[i] -> buffer);
            frames_GOOD[i] -> buffer = NULL;
            free(frames_GOOD[i]);
        }
    }

    // Cleanup
    VmbCameraClose(cameraHandle);
    VmbShutdown();

    return error;
}

VmbError_t StopStreaming(VmbHandle_t handle)
{
    //Vimba Parameters
    VmbError_t error = VmbErrorSuccess;
    
    printf("Stoping Acquisition...\n");
    // Stop Acquisition
    error = VmbFeatureCommandRun( handle,"AcquisitionStop" );
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not stop acquisition. Error code: %d\n", error );
        VmbCaptureEnd(handle);
        VmbCameraClose(handle);
        VmbShutdown();
        return 1;
    }
    
    printf("Video streaming ended\n");

    // Cleanup
    VmbCaptureEnd(handle);
    VmbCaptureQueueFlush(handle);
    VmbCameraClose(handle);
    VmbShutdown();

    return error;
}

VmbError_t StartStreaming()
{
    //Socket Parameters
    int                 server_socket;
    int                 socket_buffer_size      = UDP_PACKET_SIZE;
    struct sockaddr_in  server_address;
    struct sockaddr_in  client_address;
    socklen_t           client_address_len      = sizeof(client_address);
    
    int     bytes_recv                          = 0;
    char*   telemetry                           = (char*)(malloc(MAX_BUFFER_SIZE_OPT));
    

    //Destinatary IP for frames
    const char*         client_ip               = "192.168.10.33";
    int                 client_port             = CLIENT_PORT;
    int                 server_port             = SERVER_PORT;
    VmbInt64_t          MulticastIPAddress      = 4026531834;
                                        
    // Set client address
    client_address.sin_addr.s_addr  = inet_addr(client_ip);                     // Server IP (Raspberry Pi)
    client_address.sin_family       = PF_INET;
    client_address.sin_port         = htons(client_port);

    server_address.sin_family       = PF_INET;
    server_address.sin_addr.s_addr  = htonl(INADDR_ANY);
    server_address.sin_port         = htons( server_port );

    // Create server socket
    if ((server_socket = socket(PF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_socket, SOL_SOCKET, SO_SNDBUF, (char *) &socket_buffer_size, sizeof(socket_buffer_size)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Bind server socket to port and listen any address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(struct sockaddr_in)) != 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    //Vimba Parameters
    VmbError_t          error               = VmbErrorSuccess;
    VmbAccessMode_t     cameraAccessMode    = VmbAccessModeFull;    // We open the camera with full access
    VmbBool_t           bIsCommandDone      = VmbBoolFalse;         // Has a command finished execution
    VmbHandle_t         cameraHandle        = NULL;

    printf("Configuring the camera...\n");

    // Initialize Vimba
    error = VmbStartup();
    if (error != VmbErrorSuccess)
    {
        printf("Error initializing Vimba: %d\n", error);
        return 1;
    }
    
    //Ping GigE camera
    error = VmbFeatureCommandRun( gVimbaHandle, "GeVDiscoveryAllOnce" );
    if( error != VmbErrorSuccess )
    {
        printf( "Could not ping GigE cameras over the network. Reason: %d\n", error );
    }

    // Open camera
    error = VmbCameraOpen("DEV_000F315DF05B", cameraAccessMode, &cameraHandle);
    if (error != VmbErrorSuccess)
    {
        printf("Error opening camera: %d\n", error);
        VmbShutdown();
        return 1;
    }

    //Set multicast mode
    error = VmbFeatureBoolSet(cameraHandle, "MulticastEnable", true);
    if (error != VmbErrorSuccess)
    {
        printf("Error setting multicast mode: %d\n", error);
        return 1;
    }
    //Set multicast IP
    error = VmbFeatureIntSet(cameraHandle, "MulticastIPAddress", MulticastIPAddress);
    if (error != VmbErrorSuccess)
    {
        printf("Error setting multicast IP address: %d\n", error);
        return 1;
    }
    
    // Set the GeV packet size to the highest possible value
    if ( VmbErrorSuccess == VmbFeatureCommandRun(cameraHandle, "GVSPAdjustPacketSize"))
    {
        do
        {
            if ( VmbErrorSuccess != VmbFeatureCommandIsDone(cameraHandle, "GVSPAdjustPacketSize", &bIsCommandDone))
            {
                break;
            }
        } while ( VmbBoolFalse == bIsCommandDone );
    }
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not adjust packet size. Error code: %d\n", error );
        VmbShutdown();
        return 1;
    }
    
    // Set pixel format
    error = VmbFeatureEnumSet( cameraHandle, "PixelFormat", "BGR8Packed" );
    //error = VmbFeatureEnumSet( cameraHandle, "PixelFormat", "RGB8Packed" );
    
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not set pixel format to either RGB. Error code: %d\n", error );
        VmbShutdown();
        return 1;
    }
    
    //Set exposure mode to continuous
    error = VmbFeatureEnumSet(cameraHandle, "ExposureAuto", "Continuous");
    if (error != VmbErrorSuccess)
    {
        printf("Error setting exposure mode: %d\n", error);
        return 1;
    }

    //Set Balance White to auto
    error = VmbFeatureEnumSet(cameraHandle, "BalanceWhiteAuto", "Continuous");
    if (error != VmbErrorSuccess)
    {
        printf("Error setting exposure mode: %d\n", error);
        return 1;
    }

    //Set gain mode to continuous
    error = VmbFeatureEnumSet(cameraHandle, "GainAuto", "Continuous");
    if (error != VmbErrorSuccess)
    {
        printf("Error setting gain mode: %d\n", error);
        return 1;
    }

    //set Acquisition mode
    error = VmbFeatureEnumSet( cameraHandle , "AcquisitionMode", "Continuous" );
    if (error != VmbErrorSuccess)
    {
        printf("Error setting Acquisition mode to single frame: %d\n", error);
        return 1;
    }
 
    //Frame dimensions
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "Height", HEIGHT ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "Width", WIDTH ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //OffsetX
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetX", OFFSET_X ))
    {
        printf( "Error setting OffsetX: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //OffsetX
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetY", OFFSET_Y ))
    {
        printf( "Error setting OffsetY: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Bottom
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionBottom", DSP_SUBREGION_BOTTOM ))
    {
        printf( "Error setting DSP_SUBREGION_BOTTOM: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Left
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionLeft", DSP_SUBREGION_LEFT ))
    {
        printf( "Error setting DSP_SUBREGION_LEFT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Top
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionTop", DSP_SUBREGION_TOP ))
    {
        printf( "Error setting DSP_SUBREGION_TOP: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion Right
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionRight", DSP_SUBREGION_RIGHT ))
    {
        printf( "Error setting DSP_SUBREGION_RIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //Exposure Auto Max
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "ExposureAutoMax", EXPOSURE_AUTO_MAX ))
    {
        printf( "Error setting EXPOSURE_AUTO_MAX: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //StreamBytesPerSecond (x2)
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "StreamBytesPerSecond", 2*STREAM_BYTES_PER_SECOND ))
    {
        printf( "Error setting StreamBytesPerSecond: %d\n", error );
        VmbShutdown();
        return 1;
    }

    // Start Capture Engine
    error = VmbCaptureStart(cameraHandle);
    if (error != VmbErrorSuccess) {
        printf("Error starting capture: %d\n", error);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    // Start Acquisition
    error = VmbFeatureCommandRun( cameraHandle,"AcquisitionStart" );
    if (error != VmbErrorSuccess) {
        printf("Error startig acquisition: %d\n", error);
        VmbCaptureEnd(cameraHandle);
        VmbCameraClose(cameraHandle);
        VmbShutdown();
        return 1;
    }
    printf("Streaming video...\n");
    
    //Send Acquisition started confirmation to client
    char confirmation[MAX_BUFFER_SIZE_OPT] = {0};
    strcpy(confirmation, "ACQUISITION_POWER_ON_DONE");
    if (sendto(server_socket, confirmation, strlen(confirmation), 0, (struct sockaddr*)&client_address, sizeof(client_address)) < 0)
    {
        printf("Acquisition check sending failed\n");
        exit(EXIT_FAILURE);
    }

    //Wait for STOP telecomand from client
    bytes_recv = recvfrom(server_socket, telemetry, sizeof(telemetry), 0, (struct sockaddr *)&client_address, &client_address_len);
    if (bytes_recv == -1)
    {
        perror("Error getting client option\n");
        exit(EXIT_FAILURE);
    }
    
    error = StopStreaming(cameraHandle);
    if (error != VmbErrorSuccess)
    {
        printf("Error stopping streaming: %d\n", error);
        return 1;
    }
    free(telemetry);

    sleep(1);
    //Close socket
    if (close(server_socket) < 0)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }

    return error;
}

