

#include <stdio.h>      // Biblioteca estándar de entrada/salida en C
#include <stdlib.h>     // Biblioteca estándar de utilidades generales    
#include <stdbool.h>    // Introduce el tipo booleano (bool)
#include <unistd.h>     // Proporciona acceso a la API POSIX del sistema operativo
#include <sys/socket.h> // Proporciona las funciones y definiciones necesarias para la creación y manipulación de sockets
#include <netinet/in.h> // Contiene estructuras y constantes necesarias para especificar direcciones IP
#include <sys/types.h>  // Biblioteca de la que dependen 'sys/socket.h' y 'netinet/in.h' 
#include <arpa/inet.h>  // Proporciona funciones para la manipulación de direcciones de red
#include <netdb.h>      // Proporciona definiciones para el acceso a la base de datos de red
#include <string.h>     //  Biblioteca que contiene funciones para el manejo de cadenas de caracteres



#include "../include/camera_handler.h"
#include "../include/pruebas_server.h"

#define CLIENT_PORT                 28000
#define SERVER_PORT                 8080
#define PROCESSOR_PORT              26000

#define MAX_BUFFER_SIZE_OPT         1024

#define HEIGHT                      1024
#define WIDTH                       1024

#define OFFSET_X                    512
#define OFFSET_Y                    512

#define DSP_SUBREGION_BOTTOM        2048
#define DSP_SUBREGION_LEFT          0
#define DSP_SUBREGION_RIGHT         2048
#define DSP_SUBREGION_TOP           0

#define EXPOSURE_AUTO_MAX           1000000/20                  //Garantizar 20 fps

#define STREAM_BYTES_PER_SECOND     32000000                    //32 Mbs (max. Raspberry Pi can handle is 32Mbps)

#define UDP_PACKET_SIZE             49152

#define NUM_FRAMES                  15
#define NUM_FRAMES_HDR              5*4*5*5*4

#define MIN_FRAMES                  10
#define HDR_FRAMES                  3
#define GOOD_FRAMES                 3
#define TOTAL_FRAMES                MIN_FRAMES + HDR_FRAMES + GOOD_FRAMES

#define THRESHOLD_MAX               63

#define TIMEOUT                     2000


int main(int argc, char* argv[])
{
    VmbError_t error = VmbErrorSuccess;
    
    system("clear");
    //BUCLE PRINCIPAL
    while (true)
    {   
        char*   s_opt   = Get_Telecomand();
  
        //Opcion = 0 - Client disconnected
        if (strcmp(s_opt, "0") == 0)
        {
            printf("Cliente ha finalizado la sesión\n");
            //system("clear");
            break;
        }
        
        //Opcion = 1 - Send image to client
        if (strcmp(s_opt, "1") == 0)
        {       
            error = ImageAcquisition();
            if (error != VmbErrorSuccess)
            {
                printf( "Error sending good image: %d\n", error );
                return 1;
            }
        }

        //Opcion = 2 - Video streaming (multicast) (Opcion 3 inside)
        if (strcmp(s_opt, "2") == 0)
        {
            error = StartStreaming();
            if (error != VmbErrorSuccess)
            {
                printf( "Error starting streaming: %d\n", error );
                return 1;
            }
        }

        //Opcion = 3 - Send Feature List to Client
        if (strcmp(s_opt, "3") == 0)
        {
            error = GetFeatures();
            if (error != VmbErrorSuccess)
            {
                printf( "Error sending Feature List to Client: %d\n", error );
                return 1;
            }
        }

        //Opcion = 5 - Send a set of frames , with different HDR configurations (2 Knee Point), to the client
        if (strcmp(s_opt, "5") == 0)
        {
            error = N_HDR_frames_2KP();
            if (error != VmbErrorSuccess)
            {
                printf( "Error sending N HDR frames with 2 knee point: %d\n", error );
                return 1;
            }
        }

        //Opcion = 6 - Send a set of frames , with different White Balance configurations, to the client
        if (strcmp(s_opt, "6") == 0)
        {
            error = WhiteBalance_Scan();
            if (error != VmbErrorSuccess)
            {
                printf( "Error in WhiteBalance_Scan: %d\n", error );
                return 1;
            }
        }
  
        free(s_opt);      

    }
    return error;
}
