/*           
        Para compilar incluir la librería  m (matemáticas)

        Ejemplo:
            gcc -o mercator mercator.c  -lm
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>

#include <string.h>
#include <fcntl.h>
#include <mqueue.h>
#include <sys/wait.h>

#define NPROCS 4
#define SERIES_MEMBER_COUNT 20000

#define FLAGS 0                 // Default
#define MAXMSG 2                // Máximo un mensaje en el buzón //segun yo no deberia haber razon para mas de dos mensajes
#define MSGSIZE 129             // Tamaño máximo del mensaje espero que sea de bytes y no de bites, por que si no valio
#define CURMSGS 0               // Mensajes actuales en el buzón
struct mq_attr attr = {FLAGS,MAXMSG,MSGSIZE,CURMSGS};


double get_member(int n, double x){
    int i;
    double numerator = 1;

    for(i=0; i<n; i++ )
        numerator = numerator*x;

    if (n % 2 == 0)
        return ( - numerator / n );
    else
        return numerator/n;
}

void proc(int proc_num, mqd_t queue_id){
    
    //if we receive a message, begin f
    char mensaje[MSGSIZE]; //16
    int prority;
    double value_to_calculate = 0;
    if(mq_receive(queue_id,mensaje,attr.mq_msgsize,&prority)==-1){
        fprintf(stderr,"\t\t\tError al recibir valor inicial como en el archivo de txt\n");
        printf("thus everything afterthis is gonna be wrong\n");

    }
    else{
        printf("\t\tm inicial String[%s]\n",mensaje);     // Imprimir el mensaje
        value_to_calculate = atof(mensaje); 
        printf("\t\tm inicial Float[%f]\n",value_to_calculate);     // Imprimir el mensaje
    }
        
    double sum = 0;

    for(int i=proc_num; i<SERIES_MEMBER_COUNT;i+=NPROCS)
        sum += get_member(i+1, value_to_calculate); 

    // Incrementa la variable proc_count que es la cantidad de procesos que terminaron su cálculo
    //send our sum as a message

    //this could pose a proble, if we try to do the sequence with a little amount of iterations
    //as there is one message queue, and somehow we could send incorrect values

    
    sprintf(mensaje,"%lf", sum);
    if(mq_send(queue_id, mensaje,attr.mq_msgsize,0)==-1)
        fprintf(stderr,"Error al mandar sumas de vuelta\n"); 

    exit(0);
}

void master_proc(mqd_t queue_id){

    double value_to_calculate;

    // Obtener el valor de x desde el archivo entrada.txt
    FILE *fp = fopen("entrada.txt","r");
    if(fp==NULL)
        exit(1);
    
    fscanf(fp,"%lf",&value_to_calculate);
    fclose(fp);

    char mensaje[MSGSIZE]; //i think as long as the size is enough we good
    sprintf(mensaje,"%lf", value_to_calculate);
    for(int i = 0; i < NPROCS; i++) //send 4 messages
        if(mq_send(queue_id, mensaje,attr.mq_msgsize,0)==-1)
            fprintf(stderr,"Error al mandar mensaje\n"); 






    // Espera a que todos los procesos terminen su cálculo
    double total_sum = 0;
    for(int i = 0; i < NPROCS; i++){
        //if we receive a message one process has finished
        int prority;
        double total_from_a_process = 0;
        if(mq_receive(queue_id,mensaje,attr.mq_msgsize,&prority)==-1){
            fprintf(stderr,"\t\t\tError al recibir valor inicial como en el archivo de txt\n");
            printf("thus everything afterthis is gonna be wrong\n");

        }
        else{
            printf("\t\tm inicial String[%s]\n",mensaje);     // Imprimir el mensaje
            total_from_a_process = atof(mensaje); 
            printf("\t\tm inicial Float[%f]\n",total_from_a_process);     // Imprimir el mensaje
        }
        total_sum += total_from_a_process;
    }

    // Send back the total value
    
    sprintf(mensaje,"%lf", total_sum);
    if(mq_send(queue_id, mensaje,attr.mq_msgsize,0)==-1)
        fprintf(stderr,"Error al mandar TOTAAL de sumas de vuelta\n"); 

    exit(0);
}

int main(){

    int *threadIdPtr;
    
    long long start_ts;
    long long stop_ts;
    long long elapsed_time;
    long lElapsedTime;
    struct timeval ts;
    int i;
    int p;
    int status; 


    //preparar lo de los semaforos
    mqd_t queue_id;      // Buzón de mensajes
    char queue[] = "/mqueue1"; //this should work?
    mq_unlink(queue); //erases any previous message that could have stayed in the queue
                    //open the semaphore and save teh queue id
    queue_id = mq_open(queue ,O_RDWR | O_CREAT, 0666, &attr);
    if(queue_id==-1)
        fprintf(stderr,"Error al crear la cola de mensajes [%d]\n", i);
        
    





    gettimeofday(&ts, NULL);
    start_ts = ts.tv_sec; // Tiempo inicial

    for(i=0; i<NPROCS;i++)
    {
        p = fork();
        if(p==0)
            proc(i, queue_id);
    }

    p = fork();
    if(p==0)
        master_proc(queue_id);

    printf("El recuento de ln(1 + x) miembros de la serie de Mercator es %d\n",SERIES_MEMBER_COUNT);
    
    for(int i=0;i<NPROCS+1;i++)
    {
        wait(&status);
        if(status==0x100)   // Si el master_proc termina con error
        {
            fprintf(stderr,"Proceso no puede abrir el archivo de entrada\n");
            break;
        }
    }
    
    //because of wait we can simply just recive messaje here.
    char mensaje[MSGSIZE]; 
    int prority;
    double total;
    if(mq_receive(queue_id,mensaje,attr.mq_msgsize,&prority)==-1){
        fprintf(stderr,"\t\t\tError al recibir valor FINAL \n");
        printf("thus everything afterthis is gonna be wrong\n");

    }
    else{
        printf("\t\tm total String[%s]\n",mensaje);     // Imprimir el mensaje
        total = atof(mensaje); 
        printf("\t\tm total Float[%f]\n",total);     // Imprimir el mensaje
    }

    gettimeofday(&ts, NULL);
    stop_ts = ts.tv_sec; // Tiempo final
    elapsed_time = stop_ts - start_ts;
    
    printf("Tiempo = %lld segundos\n", elapsed_time);
    printf("El resultado es %10.8f\n", total);
    printf("Llamando a la función ln(1 + %f) = %10.8f\n",shared->x_val, log(1+shared->x_val));

    //at the end do this f***er
    //we could maybe ask chat later, if we can send directly a float
    mq_close(queue_id);
    mq_unlink(queue);
}
