#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include "ringbuf.h"
#include "comm/rs232.h"
#include "types_donnees.h"

#define DEBUG_MSG (0)
#define TAILLE_TXBUFFER (2048)

pthread_t communication_task ;
pthread_t processing_task ;
int comm_port ;

char tx_ring_data[TAILLE_TXBUFFER];
sem_t mutex;
sem_t datamutex ;

ringbuffer txbuffer ;
static const function_list_entry Wrapper_funcs[] ;

struct Position {
    long date_maj ;
    long version ;
    double lat ;
    double lon ;
};

struct Position pos ;

struct EntreeAnalogique {
    long date_maj ;
    long version ;
    double valeur ;
};
struct EntreeAnalogique A0 ;

#define TAILLE_BUFFER (1024)

void decode_gps( long arduinoTime, json_t *pt ) {
    double lat, lon ;
    if( pt == NULL ) {
        return ;
    }
    if( !json_is_array(pt)) {
        fprintf(stderr, "%s() erreur un tableau de valeurs etait attendu\n", __func__ );
        return ;
    }

    int valeurs = (int)json_array_size( pt ) ;
    if( valeurs != 2 ) {
        fprintf(stderr, "%s() erreur un tableau de 2 valeurs etait attendu, recu:%d\n", __func__, valeurs);
        return ;
    }

    lat = json_number_value( json_array_get(pt, 0) );
    lon = json_number_value( json_array_get(pt, 1) );
    printf("%s: position GPS reçue : %.6f, %.6f\n", __func__, lat, lon);
    // prendre accès exclusif
    sem_wait(&datamutex );
    pos.lat = lat ;
    pos.lon = lon ;
    pos.date_maj = arduinoTime ;
    pos.version++ ;
    sem_post(&datamutex);
}

void decode_analog( long arduinoTime, json_t *pt ) {
    double x ;
    if( pt == NULL ) {
        return ;
    }
    if( !json_is_array(pt)) {
        fprintf(stderr, "%s() erreur un tableau de valeurs etait attendu\n", __func__ );
        return ;
    }

    int valeurs = (int)json_array_size( pt ) ;
    if( valeurs != 1 ) {
        fprintf(stderr, "%s() erreur un tableau de 1 valeur etait attendu, recu:%d\n", __func__, valeurs);
        return ;
    }

    x = json_number_value( json_array_get(pt, 0) );
    printf("%s: valeur reçue : %.6f \n", __func__, x);
    sem_wait(&datamutex );
    A0.date_maj = arduinoTime ;
    A0.version++ ;
    A0.valeur = x ;
    sem_post(&datamutex);
}

void decodeJSon( char *sourceJSON ) {
    json_error_t error;
    json_t *root ;

    if( sourceJSON == NULL ) {
        fprintf(stderr, "%s() error src NULL\n", __func__  );
        return ;
    }
    root = json_loads(sourceJSON, 0, &error);
    if( !root ) {
        fprintf(stderr, "%s() error %s\n", __func__, sourceJSON );
        return ;
    }
    json_t *pt = json_object_get( root, (char *)"msg_id" );
    if( pt == NULL ) {
        fprintf( stderr,"%s : JSon erreur [msg_id] : %s\n", __func__, sourceJSON );
        fflush(stderr);
        json_decref(root);
        return ;
    }

    long msg_id = (long)json_integer_value( pt );
    pt = json_object_get( root, (char *)"time" );
    if( pt == NULL ) {
        fprintf( stderr,"%s : JSon erreur [time] : %s\n", __func__, sourceJSON );
        fflush(stderr);
        json_decref(root);
        return ;
    }
    long msg_time = (long)json_integer_value( pt );
    pt = json_object_get( root, (char *)"sensor" );
    if( pt == NULL ) {
        fprintf( stderr,"%s : JSon erreur [sensor] : %s\n", __func__, sourceJSON );
        fflush(stderr);
        json_decref(root);
        return ;
    }
    char *sensor = (char *)json_string_value(pt);

    printf("Message %ld date de %ld capteur : %s\n", msg_id, msg_time, sensor );

    pt = json_object_get( root, (char *)"data" );
    if( pt == NULL ) {
        fprintf( stderr,"%s : JSon erreur [sensor] : %s\n", __func__, sourceJSON );
        fflush(stderr);
        json_decref(root);
        return ;
    }

    // chercher quelle fonction peut traiter ce message
    int called = 0 ;
    function_list_entry* start = Wrapper_funcs ;
    while( start != NULL ) {
        if( strcmp( start->key, sensor) == 0 ) {
            (*start->ptr)(msg_time,pt);
            called = 1 ;
            break ;
        }
        start++ ;
    }
    if( !called ) {
        fprintf( stderr,"%s : JSon erreur pas de fonction pour capteur: %s\n", __func__, sensor );
        fflush(stderr);
    }
    json_decref(root);
}


#define SEPARATEUR '\n'
void* communication( void *params ) {
    int n,i,j,premier ;
    ringbuffer rb ;
    char ring_data[TAILLE_BUFFER];
    unsigned char rs232_buffer[TAILLE_BUFFER] ;
    char json_message[TAILLE_BUFFER];
    char *c ;
    rbinit( &rb, ring_data, TAILLE_BUFFER);
    json_message[0] = (char)0 ;
    j = 0 ;
    premier = 0 ;
    for( ; ; ) {
        n = RS232_PollComport( comm_port, rs232_buffer, TAILLE_BUFFER);
        for( i=0 ; i < n ; i++ )
            rbput( &rb, rs232_buffer[i]);

        // reception des donnees
        while( !rbisempty(&rb)) {
            if( !premier ) {
                c=rbget(&rb);
                if( *c == SEPARATEUR )
                    premier = 1 ;
            } else {
                c=rbget(&rb);

                if( *c == SEPARATEUR)  {
                    if( DEBUG_MSG ) printf("Message : %s\n", json_message);
                    decodeJSon( json_message );
                    fflush(stdout);
                    j=0 ;
                } else {
                    json_message[j++] = *c ;
                    json_message[j] = 0 ;
                }
            }
        }
        // envoi
        // prendre accès exclusif au buffer d'envoi
        sem_wait( &mutex );
        if( !rbisempty(&txbuffer) ) {
            while( !rbisempty(&txbuffer)) {
                c=rbget(&txbuffer);
                RS232_SendBuf( comm_port, (unsigned char *)c, 1 ); // pas optimal... envoi 1 par 1
            }
        }
        sem_post(&mutex);
    }
    return((void *)NULL);
}


static const function_list_entry Wrapper_funcs[] = {
    { "gps", decode_gps  },
    { "analog", decode_analog },
    { NULL, NULL  }
};


void send_message( int led_on_off ) {
    char *ret_strings = NULL;

    json_t *root = json_object();
    json_object_set_new( root, "command", json_string( "led_status" ) );
    json_object_set_new( root, "value", json_integer(led_on_off) );
    ret_strings = json_dumps( root, 0 );
    json_decref( root );

    printf("Message envoye:%s\n", ret_strings);
    sem_wait( &mutex );
    for( int i=0 ; i < strlen(ret_strings) ; i++ ) {
         rbput( &txbuffer, ret_strings[i]);
    }
    sem_post(&mutex);
    free( ret_strings);
}


void* tache_calcul( void* p ) {
    struct EntreeAnalogique local ;
    local.date_maj = 0 ;
    local.version = 0 ;
    local.valeur = 0 ;

    printf("Debut tache calcul\n");

    int nouveau_analogique = 0 ;
    for( ; ; ) {
        // voir si changement
        sem_wait(&datamutex );
        if( A0.version != local.version ) {
            memcpy( (void *)&local, (void *)&A0, sizeof( struct EntreeAnalogique));
            nouveau_analogique = 1 ;
        }
        sem_post(&datamutex);
        if( nouveau_analogique == 0 ) {
            // pas de changement... attendre :-(
            usleep( 20000 ); // 20millisecondes
            continue ;
        }
        // traitement bidon : si valeur > 200 alors allumer led arduino
        // sinon eteindre
        nouveau_analogique = 0 ;
        if( local.valeur > 200 ) {
            send_message(1);
        } else {
            send_message(0);
        }
    }
    return((void *)NULL);
}


int main(int argc, char *argv[])
{
    char mode[]={'8','N','1',0};
    int comm_speed = 19200 ;
    comm_port = 6 ;

    if( RS232_OpenComport(comm_port,comm_speed,mode) != 0 ) {
        fprintf( stderr, "impossible d'ouvrir le port de communication %d\n", comm_port)  ;
        return(-1);
    }

    // init divers
    A0.date_maj = 0 ;
    A0.valeur = 0 ;
    A0.version = 0 ;
    sem_init(&mutex, 0, 1);

    // init du buffer
    rbinit( &txbuffer, tx_ring_data, TAILLE_TXBUFFER);
    sem_init(&datamutex, 0, 1);

    // creer la tache de communication
    pthread_create(&communication_task,
                            NULL,
                            communication,
                            NULL );
    // tache de calcul
    pthread_create(&processing_task,
                   NULL,
                   tache_calcul,
                   NULL );

    // attendre la fin...
    (void) pthread_join(communication_task, NULL);

    return 0;
}
