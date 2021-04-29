
/*
 *  Copyright 2020-2021 ARCOS.INF.UC3M.ES
 *
 *  This file is part of Operaring System Labs (OSL).
 *
 *  OSL is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  OSL is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with OSL.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include "peticion.h"

#define MAX_BUFFER 128
peticion_t  buffer[MAX_BUFFER];

int  n_elementos  = 0;
int  pos_servicio = 0;
int  fin = 0;
pthread_mutex_t  mutex;
pthread_cond_t     no_lleno;
pthread_cond_t     no_vacio;

void * receptor (void * param)
{
      const int MAX_PETICIONES = 5;
      peticion_t p;
      int i, pos=0;

     for (i=0;i<MAX_PETICIONES;i++) 
     {
          recibir_peticion(&p);
          pthread_mutex_lock(&mutex);
          while (n_elementos == MAX_BUFFER) 
                 pthread_cond_wait(&no_lleno, &mutex); 
          buffer[pos] = p;
          pos = (pos+1) % MAX_BUFFER;
          n_elementos++;
          pthread_cond_signal(&no_vacio);
          pthread_mutex_unlock(&mutex);
     }
  
       fprintf(stderr,"Finalizando receptor\n");

       pthread_mutex_lock(&mutex);
       fin=1;
       pthread_cond_broadcast(&no_vacio);
       pthread_mutex_unlock(&mutex);

       fprintf(stderr, "Finalizado receptor\n");
       pthread_exit(0);
       return NULL;

}  /* receptor  */


void * servicio (void * param) 
{
      peticion_t p;

      for (;;)
      {
           pthread_mutex_lock(&mutex);
           while (n_elementos == 0)
	   {
                if (fin==1) {
                     fprintf(stderr,"Finalizando servicio\n");
                     pthread_mutex_unlock(&mutex);
                     pthread_exit(0);
                }
                pthread_cond_wait(&no_vacio, &mutex);
           } // while



          fprintf(stderr, "Sirviendo posicion %d\n", pos_servicio);
          p = buffer[pos_servicio];
          pos_servicio = (pos_servicio + 1) % MAX_BUFFER;
          n_elementos--;
          pthread_cond_signal(&no_lleno);
          pthread_mutex_unlock(&mutex);
          responder_peticion(&p);
    }

    pthread_exit(0);
    return NULL;
}

int main ( int argc, char *argv[] ) 
{
    struct timeval timenow;
    long t1, t2;
    const int MAX_SERVICIO = 5;
    pthread_t thr;
    pthread_t ths[MAX_SERVICIO];

    // inicializar
    pthread_mutex_init(&mutex,NULL);
    pthread_cond_init(&no_lleno,NULL);
    pthread_cond_init(&no_vacio,NULL);

    for (int i=0;i<MAX_SERVICIO;i++) {
         pthread_create(&ths[i],NULL,servicio,NULL);
    }
    sleep(1);

    // t1
    gettimeofday(&timenow, NULL) ;
    t1 = (long)timenow.tv_sec * 1000 + (long)timenow.tv_usec / 1000 ;

    // receptor...
    pthread_create(&thr,NULL,receptor,NULL);
    pthread_join(thr, NULL);
    for (int i=0;i<MAX_SERVICIO;i++) {
         pthread_join(ths[i],NULL);
    }

    // t2
    gettimeofday(&timenow, NULL) ;
    t2 = (long)timenow.tv_sec * 1000 + (long)timenow.tv_usec / 1000 ;

    // finalizar
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&no_lleno);
    pthread_cond_destroy(&no_vacio);

    // imprimir t2-t1...
    printf("Tiempo total: %lf\n", (t2-t1)/1000.0);
    return 0;
}

