#ifndef F_PRUEBAS_SERVER
#define F_PRUEBAS_SERVER

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <string.h>
#include <math.h>

#include <VimbaC/Include/VimbaC.h>

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

/**
 * @brief   Obtención y envío al cliente de todas las imagenes que constituyen el conjunto de muestras para la optimización del modo HDR
 * 
 * @details Se abre un socket UDP, y la cámara con la API de Vimba. Se aplica la configuración de features para la obtención de una 
 *          imagen de contexto. Se definen los posibles valores que pueden tener de los Knee Points del modo HDR, y se itera cada una 
 *          de las posibles configuraciones. En cada iteración se calcula el tiempo de exposición de manera automática, se aplica el 
 *          algoritmo definido para una configuración del modo HDR y se envía la correspondiente imagen al cliente. Al finalizar el bucle 
 *          de iteraciones se cierra el socket UDP y se cierra la API de Vimba.
 * 
 * @return  El código de error asociado a la API de Vimba, '0' si no ha habido fallos
*/
VmbError_t N_HDR_frames_2KP();

/**
 * @brief   Obtención y envío al cliente de todas las imagenes que constituyen el conjunto de muestras para la optimización de 
 *          las ganancias del balance de blancos
 * 
 * @details Se abre un socket UDP, y la cámara con la API de Vimba. Se aplica la configuración de features para la obtención de una 
 *          imagen de contexto. Se definen los posibles valores que pueden las ganancias que constituyen la configuración del balance de blancos
 *          y se realiza una iteración por cada posible conjunto de ganancias. En cada iteración se obtiene una imagen de contexto con una 
 *          configuración de balance de blancos y se envía al cliente. Al finalizar el bucle de iteraciones se cierra el socket UDP y se cierra 
 *          la API de Vimba.
 *   
 * 
 * @return  El código de error asociado a la API de Vimba, '0' si no ha habido fallos
*/
VmbError_t WhiteBalance_Scan();

#endif