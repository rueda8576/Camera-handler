#include "../include/pruebas_server.h"

VmbError_t N_HDR_frames_2KP()
{
    //Socket Parameters
    int                 server_socket;
    int                 socket_buffer_size      = UDP_PACKET_SIZE;
    struct sockaddr_in  server_address;
    struct sockaddr_in  client_address;
    socklen_t           client_address_len      = sizeof(client_address);

    //Destinatary IP for frames
    const char*         client_ip               = "192.168.1.33";
    int                 client_port             = CLIENT_PORT;
    int                 server_port             = SERVER_PORT;

    // Set client address
    client_address.sin_addr.s_addr              = inet_addr(client_ip);                     // Server IP (Raspberry Pi)
    client_address.sin_family                   = PF_INET;
    client_address.sin_port                     = htons(client_port);
    server_address.sin_family                   = PF_INET;
    server_address.sin_addr.s_addr              = htonl(INADDR_ANY);
    server_address.sin_port                     = htons( server_port );

    int     num_packets                         = 0;
    int     total_bytes_sent                    = 0;
    int     bytes_sent;
    int     bytes_recv;

    //Vimba Parameters
    VmbError_t          error                       = VmbErrorSuccess;
    VmbHandle_t         cameraHandle                = NULL;
    VmbFrame_t*         frames_AUTO [MIN_FRAMES];
    VmbFrame_t*         frames_HDR  [HDR_FRAMES];
    VmbFrame_t*         frames_GOOD [GOOD_FRAMES];
    VmbAccessMode_t     cameraAccessMode            = VmbAccessModeFull;    // We open the camera with full access
    VmbBool_t           bIsCommandDone              = VmbBoolFalse;         // Has a command finished execution
    VmbInt64_t          nPayloadSize                = 0;                    // The size of one frame


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

    //Configuración general de la cámara
    //Set multicast mode
    error = VmbFeatureBoolSet(cameraHandle, "MulticastEnable", false);
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting multicast mode: %d\n", error );
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
    //error = VmbFeatureEnumSet( cameraHandle, "PixelFormat", "RGB8Packed" );
    error = VmbFeatureEnumSet( cameraHandle, "PixelFormat", "BGR8Packed" );
    
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not set pixel format to either RGB. Error code: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //Set Balance White Config
    error = VmbFeatureEnumSet(cameraHandle, "BalanceWhiteAuto", "Off");
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting exposure mode: %d\n", error );
        return 1;
    }

    error = VmbFeatureEnumSet(cameraHandle, "BalanceRatioSelector", "Red");
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting exposure mode: %d\n", error );
        return 1;
    }

    if ( VmbErrorSuccess != VmbFeatureFloatSet( cameraHandle , "BalanceRatioAbs", 1.39 ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    error = VmbFeatureEnumSet(cameraHandle, "BalanceRatioSelector", "Blue");
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting exposure mode: %d\n", error );
        return 1;
    }

    if ( VmbErrorSuccess != VmbFeatureFloatSet( cameraHandle , "BalanceRatioAbs", 1.28 ))
    {
        printf( "Error setting HEIGHT: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //set Acquisition mode
    error = VmbFeatureEnumSet( cameraHandle , "AcquisitionMode", "MultiFrame" );
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting Acquisition mode to Multiframe: %d\n", error );
        return 1;
    }

    //set Frame Count
    error = VmbFeatureIntSet( cameraHandle , "AcquisitionFrameCount", TOTAL_FRAMES );
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting Frame Count: %d\n", error );
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

    //Offsets
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetX", OFFSET_X ))
    {
        printf( "Error setting OffsetX: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetY", OFFSET_Y ))
    {
        printf( "Error setting OffsetY: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionBottom", DSP_SUBREGION_BOTTOM ))
    {
        printf( "Error setting DSP_SUBREGION_BOTTOM: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionLeft", DSP_SUBREGION_LEFT ))
    {
        printf( "Error setting DSP_SUBREGION_LEFT: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionTop", DSP_SUBREGION_TOP ))
    {
        printf( "Error setting DSP_SUBREGION_TOP: %d\n", error );
        VmbShutdown();
        return 1;
    }
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

    //Gain
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

    //StreamBytesPerSecond
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "StreamBytesPerSecond", STREAM_BYTES_PER_SECOND ))
    {
        printf( "Error setting StreamBytesPerSecond: %d\n", error );
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

    //Valores para los distintos frames HDR
    //2 knee point
    float EXP_weights []  = {   0.2     ,   0.4     ,   0.6     ,   0.8     ,   1   ,   1.2 };
    float EXP1_weights[]  = {   0.1     ,   0.2     ,   0.3     ,   0.4                     };
    float EXP2_weights[]  = {   0.8     ,   0.7     ,   0.6     ,   0.5                     };
    int   TH1_v       []  = {   40      ,   45      ,   50      ,   55                      };
    int   TH2_v       []  = {   15      ,   20      ,   25      ,   30                      };
    
    int EXP_length  = sizeof(EXP_weights)   /sizeof(float);                                 //k1
    int EXP1_length = sizeof(EXP1_weights)  /sizeof(float);                                 //k2
    int EXP2_length = sizeof(EXP2_weights)  /sizeof(float);                                 //k3
    int TH1_length  = sizeof(TH1_v)         /sizeof(int);                                   //k4
    int TH2_length  = sizeof(TH2_v)         /sizeof(int);                                   //k5
    int num_total_frames = TH1_length*TH2_length*EXP1_length*EXP2_length*EXP_length;

    printf("num_total_frames = %d\n", num_total_frames);

    int     count                   = 0;
    int     ThresholdPWL1;
    int     ThresholdPWL2;
    double  HDR_ExposureTimeAbs;
    double  ExposureTimePWL1;
    double  ExposureTimePWL2;

    //BUCLE TOCHO
    for (int k1 = 0; k1 < EXP_length; k1++)
    {
        for (int k2 = 0; k2 < EXP1_length; k2++)
        {
            for (int k3 = 0; k3 < EXP2_length; k3++)
            {               
                for (int k4 = 0; k4 < TH1_length; k4++)
                {
                    ThresholdPWL1   =   TH1_v[k4];
                    error = VmbFeatureIntSet( cameraHandle, "ThresholdPWL1", ThresholdPWL1 );
                    if ( VmbErrorSuccess != error )
                    {
                        printf( "Error setting ThresholdPWL1 value: %d\n", error );
                        VmbShutdown();
                        return 1;
                    }

                    for (int k5 = 0; k5 < TH2_length; k5++)
                    {
                        ThresholdPWL2 = TH2_v[k5];
                        error = VmbFeatureIntSet( cameraHandle, "ThresholdPWL2", ThresholdPWL2 );
                        if ( VmbErrorSuccess != error )
                        {
                            printf( "Error setting ThresholdPWL2 value: %d\n", error );
                            VmbShutdown();
                            return 1;
                        }

                        //Calcular ExposureTime_AUTO
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
                        int     i   = 2;
                        
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
                        printf("Calculating AutoExposure...\n");
                        //2. Obtener N frames para estabilizar ExposureTimeAbs
                        for (; i < MIN_FRAMES; i++)
                        {   
                            printf("i = %d ", i);
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
                                    printf("-");
                                    break;
                                }
                                else { printf("."); }
                            }
                            printf("\n");
                            
                            error = VmbFeatureFloatGet( cameraHandle, "ExposureTimeAbs", &ExposureTime_AUTO[i] );
                            if ( VmbErrorSuccess != error )
                            {
                                printf( "Error getting ExposureTime_AUTO value: %d\n", error );
                                VmbShutdown();
                                return 1;
                            }

                            if (ExposureTime_AUTO[i] == ExposureTime_AUTO[i-1] && ExposureTime_AUTO[i] == ExposureTime_AUTO[i-2])
                            {
                                printf("ExposureTime_AUTO = %f\n", ExposureTime_AUTO[i]);
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

                        printf("ExposureTime_AUTO_FINAL = %f\n", ExposureTime_AUTO[i-1]);

                        //4. Configurar AutoExposure Off
                        if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureAuto", "Off" ) )
                        {
                            printf( "Error setting ExposureAuto: %d\n", error );
                            VmbShutdown();
                            return 1;
                        }

                        HDR_ExposureTimeAbs = EXP_weights [k1] * ExposureTime_AUTO[i-1];
                        ExposureTimePWL1    = EXP1_weights[k2] * HDR_ExposureTimeAbs;
                        ExposureTimePWL2    = EXP2_weights[k3] * HDR_ExposureTimeAbs;
                        
                        //5. Configurar ExposureTimeAbs para HDR
                        error = VmbFeatureFloatSet( cameraHandle, "ExposureTimeAbs", HDR_ExposureTimeAbs );
                        if ( VmbErrorSuccess != error )
                        {
                            printf( "Error setting ExposureTimeAbs value: %d\n", error );
                            VmbShutdown();
                            return 1;
                        }
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
                        if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureMode", "PieceWiseLinearHDR" ) )
                        {
                            printf( "Error setting ExposureMode to PieceWiseLinearHDR: %d\n", error );
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

                        printf("Stabilizing HDR...\n");
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
                        printf("Taking final frame...\n");
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

                        char    s_ExposureTime_AUTO[100]   = {0};
                        sprintf(s_ExposureTime_AUTO, "%f", ExposureTime_AUTO[i-1]);
                        //Send ExposureTimeAbs to client
                        if (sendto(server_socket, s_ExposureTime_AUTO, strlen(s_ExposureTime_AUTO), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
                        {
                            printf("s_ExposureTime_AUTO sending failed\n");
                            exit(EXIT_FAILURE);
                        }
       
                        printf("Sending frame...\n");
                        //Send frame.buffer (Split buffer for sending purpose)                        
                        for (int k = 0; k < frames_GOOD[num] -> bufferSize; k += UDP_PACKET_SIZE)
                        {
                            char* aux_buffer = (char*)malloc(UDP_PACKET_SIZE * sizeof(char));
                            bytes_sent = 0;
                            memcpy(aux_buffer, ((char*)frames_GOOD[num] -> buffer) + k, UDP_PACKET_SIZE);
                            bytes_sent = sendto(server_socket, aux_buffer, UDP_PACKET_SIZE, 0, (struct sockaddr *)&client_address, sizeof(client_address));
                            if (bytes_sent > 0)
                            {
                                total_bytes_sent = bytes_sent + total_bytes_sent;
                                num_packets++;
                                // printf("total_bytes_sent = %d\n", total_bytes_sent);
                                // printf("bytes_sent = %d\n", bytes_sent);
                                
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
                            free(aux_buffer);
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
                        count++;
                        printf( "count = %d\n", count );
                        
                    }
                }
            }
        }
        
        
    }
  
    printf("%d frames captured and sent to client\n", NUM_FRAMES_HDR);       

    //Close socket
    if (close(server_socket) < 0)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }
    // Cleanup
    VmbCameraClose(cameraHandle);
    VmbShutdown();
    return VmbErrorSuccess;
}

VmbError_t WhiteBalance_Scan()
{
    //Socket Parameters
    int                 server_socket;
    int                 socket_buffer_size      = UDP_PACKET_SIZE;
    struct sockaddr_in  server_address;
    struct sockaddr_in  client_address;
    socklen_t           client_address_len      = sizeof(client_address);

    //Destinatary IP for frames
    const char*         client_ip               = "192.168.1.33";
    int                 client_port             = CLIENT_PORT;
    int                 server_port             = SERVER_PORT;

    // Set client address
    client_address.sin_addr.s_addr              = inet_addr(client_ip);                     // Server IP (Raspberry Pi)
    client_address.sin_family                   = PF_INET;
    client_address.sin_port                     = htons(client_port);
    server_address.sin_family                   = PF_INET;
    server_address.sin_addr.s_addr              = htonl(INADDR_ANY);
    server_address.sin_port                     = htons( server_port );

    int     num_packets                         = 0;
    int     total_bytes_sent                    = 0;
    int     bytes_sent;
    int     bytes_recv;

    //Vimba Parameters
    VmbError_t          error                       = VmbErrorSuccess;
    VmbHandle_t         cameraHandle                = NULL;
    VmbFrame_t*         frames_AUTO [MIN_FRAMES];
    VmbFrame_t*         frames_HDR  [HDR_FRAMES];
    VmbFrame_t*         frames_GOOD [GOOD_FRAMES];
    VmbAccessMode_t     cameraAccessMode            = VmbAccessModeFull;    // We open the camera with full access
    VmbBool_t           bIsCommandDone              = VmbBoolFalse;         // Has a command finished execution
    VmbInt64_t          nPayloadSize                = 0;                    // The size of one frame


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

    //Configuración general de la cámara
    //Set multicast mode
    error = VmbFeatureBoolSet(cameraHandle, "MulticastEnable", false);
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting multicast mode: %d\n", error );
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
    //error = VmbFeatureEnumSet( cameraHandle, "PixelFormat", "RGB8Packed" );
    error = VmbFeatureEnumSet( cameraHandle, "PixelFormat", "BGR8Packed" );
    
    if ( VmbErrorSuccess != error )
    {
        printf( "Could not set pixel format to either RGB. Error code: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //Set Balance White Config
    error = VmbFeatureEnumSet(cameraHandle, "BalanceWhiteAuto", "Off");
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting exposure mode: %d\n", error );
        return 1;
    }

    //set Acquisition mode
    error = VmbFeatureEnumSet( cameraHandle , "AcquisitionMode", "MultiFrame" );
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting Acquisition mode to Multiframe: %d\n", error );
        return 1;
    }

    //set Frame Count
    error = VmbFeatureIntSet( cameraHandle , "AcquisitionFrameCount", TOTAL_FRAMES );
    if (error != VmbErrorSuccess)
    {
        printf( "Error setting Frame Count: %d\n", error );
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

    //Offsets
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetX", OFFSET_X ))
    {
        printf( "Error setting OffsetX: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "OffsetY", OFFSET_Y ))
    {
        printf( "Error setting OffsetY: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //DSP subregion
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionBottom", DSP_SUBREGION_BOTTOM ))
    {
        printf( "Error setting DSP_SUBREGION_BOTTOM: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionLeft", DSP_SUBREGION_LEFT ))
    {
        printf( "Error setting DSP_SUBREGION_LEFT: %d\n", error );
        VmbShutdown();
        return 1;
    }
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "DSPSubregionTop", DSP_SUBREGION_TOP ))
    {
        printf( "Error setting DSP_SUBREGION_TOP: %d\n", error );
        VmbShutdown();
        return 1;
    }
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

    //Gain
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

    //StreamBytesPerSecond
    if ( VmbErrorSuccess != VmbFeatureIntSet( cameraHandle , "StreamBytesPerSecond", STREAM_BYTES_PER_SECOND ))
    {
        printf( "Error setting StreamBytesPerSecond: %d\n", error );
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

    //Valores para los distintos frames
    //Solo modificar RedRatio
    float Red_Weight  []    = {0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3};
    float Blue_Weight []    = {0.8, 0.9, 1.0, 1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9, 2.0, 2.1, 2.2, 2.3, 2.4, 2.5, 2.6, 2.7, 2.8, 2.9, 3};

    int Red_length  = sizeof(Red_Weight)    /sizeof(float);         //k1
    int Blue_length = sizeof(Blue_Weight)   /sizeof(float);         //k2

    int num_total_frames = Red_length * Blue_length;
    printf("num_total_frames = %d\n", num_total_frames);

    int     count                   = 0;
    int     ThresholdPWL1;
    int     ThresholdPWL2;
    double  HDR_ExposureTimeAbs;
    double  ExposureTimePWL1;
    double  ExposureTimePWL2;

    ThresholdPWL1 = 40;
    error = VmbFeatureIntSet( cameraHandle, "ThresholdPWL1", ThresholdPWL1 );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error setting ThresholdPWL1 value: %d\n", error );
        VmbShutdown();
        return 1;
    }

    ThresholdPWL2 = 30;
    error = VmbFeatureIntSet( cameraHandle, "ThresholdPWL2", ThresholdPWL2 );
    if ( VmbErrorSuccess != error )
    {
        printf( "Error setting ThresholdPWL2 value: %d\n", error );
        VmbShutdown();
        return 1;
    }

    //BUCLE TOCHO
    for (int k1 = 0; k1 < Red_length; k1++)
    {
        for (int k2 = 0; k2 < Blue_length; k2++)
        {
            error = VmbFeatureEnumSet(cameraHandle, "BalanceRatioSelector", "Red");
            if (error != VmbErrorSuccess)
            {
                printf( "Error setting exposure mode: %d\n", error );
                return 1;
            }

            if ( VmbErrorSuccess != VmbFeatureFloatSet( cameraHandle , "BalanceRatioAbs", Red_Weight[k1] ))
            {
                printf( "Error setting HEIGHT: %d\n", error );
                VmbShutdown();
                return 1;
            }

            error = VmbFeatureEnumSet(cameraHandle, "BalanceRatioSelector", "Blue");
            if (error != VmbErrorSuccess)
            {
                printf( "Error setting exposure mode: %d\n", error );
                return 1;
            }

            if ( VmbErrorSuccess != VmbFeatureFloatSet( cameraHandle , "BalanceRatioAbs", Blue_Weight[k2] ))
            {
                printf( "Error setting HEIGHT: %d\n", error );
                VmbShutdown();
                return 1;
            }

            //Calcular ExposureTime_AUTO
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
            int     i   = 2;
            
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
            printf("Calculating AutoExposure...\n");
            //2. Obtener N frames para estabilizar ExposureTimeAbs
            for (; i < MIN_FRAMES; i++)
            {   
                printf("i = %d ", i);
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
                        printf("-");
                        break;
                    }
                    else { printf("."); }
                }
                printf("\n");
                
                error = VmbFeatureFloatGet( cameraHandle, "ExposureTimeAbs", &ExposureTime_AUTO[i] );
                if ( VmbErrorSuccess != error )
                {
                    printf( "Error getting ExposureTime_AUTO value: %d\n", error );
                    VmbShutdown();
                    return 1;
                }

                if (ExposureTime_AUTO[i] == ExposureTime_AUTO[i-1] && ExposureTime_AUTO[i] == ExposureTime_AUTO[i-2])
                {
                    printf("ExposureTime_AUTO = %f\n", ExposureTime_AUTO[i]);
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

            printf("ExposureTime_AUTO_FINAL = %f\n", ExposureTime_AUTO[i-1]);

            //4. Configurar AutoExposure Off
            if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureAuto", "Off" ) )
            {
                printf( "Error setting ExposureAuto: %d\n", error );
                VmbShutdown();
                return 1;
            }

            HDR_ExposureTimeAbs = 1.2 * ExposureTime_AUTO[i-1];
            ExposureTimePWL1    = 0.1 * HDR_ExposureTimeAbs;
            ExposureTimePWL2    = 0.5 * HDR_ExposureTimeAbs;
            
            //5. Configurar ExposureTimeAbs para HDR
            error = VmbFeatureFloatSet( cameraHandle, "ExposureTimeAbs", HDR_ExposureTimeAbs );
            if ( VmbErrorSuccess != error )
            {
                printf( "Error setting ExposureTimeAbs value: %d\n", error );
                VmbShutdown();
                return 1;
            }
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
            if ( VmbErrorSuccess != VmbFeatureEnumSet( cameraHandle, "ExposureMode", "PieceWiseLinearHDR" ) )
            {
                printf( "Error setting ExposureMode to PieceWiseLinearHDR: %d\n", error );
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

            printf("Stabilizing HDR...\n");
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
            printf("Taking final frame...\n");
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

            char    s_ExposureTime_AUTO[100]   = {0};
            sprintf(s_ExposureTime_AUTO, "%f", ExposureTime_AUTO[i-1]);
            //Send ExposureTimeAbs to client
            if (sendto(server_socket, s_ExposureTime_AUTO, strlen(s_ExposureTime_AUTO), 0, (struct sockaddr *)&client_address, sizeof(client_address)) == -1)
            {
                printf("s_ExposureTime_AUTO sending failed\n");
                exit(EXIT_FAILURE);
            }
        
            printf("Sending frame...\n");
            //Send frame.buffer (Split buffer for sending purpose)                        
            for (int k = 0; k < frames_GOOD[num] -> bufferSize; k += UDP_PACKET_SIZE)
            {
                char* aux_buffer = (char*)malloc(UDP_PACKET_SIZE * sizeof(char));
                bytes_sent = 0;
                memcpy(aux_buffer, ((char*)frames_GOOD[num] -> buffer) + k, UDP_PACKET_SIZE);
                bytes_sent = sendto(server_socket, aux_buffer, UDP_PACKET_SIZE, 0, (struct sockaddr *)&client_address, sizeof(client_address));
                if (bytes_sent > 0)
                {
                    total_bytes_sent = bytes_sent + total_bytes_sent;
                    num_packets++;
                    // printf("total_bytes_sent = %d\n", total_bytes_sent);
                    // printf("bytes_sent = %d\n", bytes_sent);
                    
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
                free(aux_buffer);
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
            count++;
            printf( "count = %d\n", count );


        }
    }
  
    printf("%d frames captured and sent to client\n", NUM_FRAMES_HDR);       

    //Close socket
    if (close(server_socket) < 0)
    {
        perror("Error closing socket\n");
        exit(EXIT_FAILURE);
    }
    // Cleanup
    VmbCameraClose(cameraHandle);
    VmbShutdown();
    return VmbErrorSuccess;
}