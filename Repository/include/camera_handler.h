#ifndef F_SERVER
#define F_SERVER

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

#include <VimbaImageTransform/Include/VmbTransform.h>
#include <VimbaC/Include/VimbaC.h>

#define CLIENT_PORT                 28000
#define SERVER_PORT                 8080
#define PROCESSOR_PORT              26000

#define MAX_BUFFER_SIZE_OPT         1024

#define PIXEL_FORMAT                "BGR8Packed"
#define HEIGHT                      1024
#define WIDTH                       1024

#define OFFSET_X                    512
#define OFFSET_Y                    512

#define GAIN_RED                    1.6
#define GAIN_BLUE                   1

#define DSP_SUBREGION_BOTTOM        2048
#define DSP_SUBREGION_LEFT          0
#define DSP_SUBREGION_RIGHT         2048
#define DSP_SUBREGION_TOP           0

#define EXPOSURE_AUTO_MAX           1000000/20                                  //Garantizar 20 fps

#define STREAM_BYTES_PER_SECOND     32000000                                    //32 Mbs (max. Raspberry Pi can handle is 32Mbps)

#define UDP_PACKET_SIZE             49152                                        

#define NUM_FRAMES                  15

#define MIN_FRAMES                  10
#define HDR_FRAMES                  3
#define GOOD_FRAMES                 3
#define TOTAL_FRAMES                MIN_FRAMES + HDR_FRAMES + GOOD_FRAMES

#define THRESHOLD_1                 40
#define THRESHOLD_2                 30

#define TIMEOUT                     2000

/**
 * 
 * @brief   Se obtiene el comando del EGSE via socket UDP
 * @details Se abre un socket UDP que queda a la espera de un comando por parte del EGSE,
 *          una vez recibido el comando este se almacena en un string, el cual es retornado por la función,
 *          una vez habiendo cerrado el socket empleado
 * @return  Un string que contiene un número entero que caracteriza la función a realizar
*/
char* Get_Telecomand();

/** 
 * @brief   Se envían una lista de todos los feautures de la cámara, junto con su valor y una breve descripción
 * @details Se abre un socket UDP, y se abre la cámara, para obtener el número total de features a obtener.
 *          El número de features totales es enviado por el socket al EGSE, y se procede a rellenar una lista de
 *          los features de la cámara. Una vez finalizada la lectura de la lista, se envía cada feature de la lista,
 *          junto con su valor y su descripción al EGSE. Al finalizar se cierra el socket
 *          por el socket
 *   
 * @return  El código de error asociado a la API de Vimba, '0' si no ha habido fallos
 */
VmbError_t GetFeatures();

/** 
 * @brief   Captura y envío al EGSE de una imagen de contexto
 * @details Se abre un socket UDP, y la cámara con la API de Vimba. Se aplica la configuración de features para la
 *          obtención de una imagen de contexto, se realizan los algoritmos de cálculo de tiempo de exposición y
 *          configuración del modo HDR, y se almacena en un buffer los datos de la imagen capturada. Se obtienen los
 *          valores de tiempo de exposición y ganancia, y se procede al envío de los datos al EGSE. Se envía primeramente
 *          los valores de tiempo de exposición y ganancia, y se procede al envío de 64 paquetes que contienen la información
 *          fragmentada del buffer de la imagen capturada. Para finalizar se cierra la API de Vimba y el socket UDP.
 *   
 * @return  El código de error asociado a la API de Vimba, '0' si no ha habido fallos
 */
VmbError_t ImageAcquisition();

/** 
 * @brief   Finaliza el proceso de streaming de video
 *
 * @details Se finaliza la adquisición de frames, y se cierra la API de Vimba.
 * @param   Camera La variable que define el handler de la cámara en uso
 * @return  El código de error asociado a la API de Vimba, '0' si no ha habido fallos
 */
VmbError_t StopStreaming(VmbHandle_t handle);

/** 
 * @brief   Inicia el proceso de streaming de video
 * 
 * @details Se abre un socket UDP y cámara con la API de Vimba. Se aplica la configuración para streaming de vídeo a la cámara,
 *          y se inicia la multidifusión de frames. Se envía un mensaje al EGSE para que inicie la caputura de frames, y se queda
 *          en espera a un mensaje del EGSE para finalizar la adquisición de frames por multidifusión. Se invoca a la función
 *          'StopStreaming' y se cierra el socket UDP.
 * @return  El código de error asociado a la API de Vimba, '0' si no ha habido fallos
 */
VmbError_t StartStreaming();

#endif