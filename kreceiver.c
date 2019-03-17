#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001

int main(int argc, char** argv) {
    
    unsigned short crc, k=0;
    init(HOST, PORT);
    msg *r;
    minikermit mk;
    //initializez datele structurii
    mk.SOH=0x01;
    mk.LEN=255;
    mk.SEQ=0;
    mk.MARK=0x0D;
    mk.CHECK=0;
    char rec[6], buffer[251];

    //Pentru SEND INIT
    while(k<3){
        r=receive_message_timeout(5000); //Verific mesajul primit
        if( r == NULL ) //pentru TIMEOUT
            k++; //incrementez k
        else { 
        	k = 0; //pentru NACK
            crc = crc16_ccitt(r->payload, r->len-4);
            unsigned short val = *((unsigned short*)(r->payload+254));
            if(val == crc) //ACK
            	k = 4; //Daca e ACK ies din while
        }
    }
    if(k == 3){ //TIMEOUT de 3 ori consecutiv 
    	printf("TIMEOUT la SEND INIT\n");
        return -1;
    }
    else { //DACA s-a primit 
        msg t;
        memcpy(t.payload, r->payload, r->len); //creez acelasi mesaj cu cel primit                
        t.len=r->len;
        sprintf(t.payload+3, "%c",'Y'); //cu tipul ACK
        send_message(&t); //il trimit
        printf("				[%d] ACK pentru [%d]\n ", t.payload[2], r->payload[2]);
    }



    k=0;
    while(1){//In while o sa primesc de fiecare data un mesaj pe care il verific
        r= receive_message_timeout(5000);
        if( r == NULL){ //daca e timeout incrementez k, si daca (jos) ajunge 3 dau return -1
            k++;
            
        } 
        else {
        	k = 0;
            crc = crc16_ccitt(r->payload, r->len-4); //fac crc mesajului primit
            unsigned short val = *((unsigned short*)(r->payload+254)); //crc-ul din CHECKUL mesajului

            if(crc != val){ //daca sunt diferite trimit N
                msg t;
                mk.TYPE='N'; //pun in structura mea la type N
                mk.SEQ=(mk.SEQ+1)%64;
                memcpy(t.payload, &mk, 258); //copiez structura in payload
                t.len=258;
                send_message(&t); //trimit mesajul
            	printf("				[%d] NACK pentru [%d]\n", t.payload[2], r->payload[2]);
            }
            else{ //daca crc-urile corespund
                msg t;
                mk.TYPE='Y'; // voi trimite ACK
                mk.SEQ=(mk.SEQ+1)%64;
                memcpy(t.payload, &mk, 258);
                t.len=258;
                unsigned char length;
                
                switch(r->payload[3]){ //si verific ce se afla in TYPE (r->payload[3]) ca receiverul sa stie ce sa faca

                    case 'F': //FISIER
                        strcpy(rec, "recv_");
                        char *name =  malloc(strlen(rec)+strlen(r->payload+4)+1); //r->payload este DATA si-anume numele fisierului trimis
                        strcpy(name, rec);			//
                        strcat(name, r->payload+4); //creez numele fisierului
                        FILE* f = fopen(name, "wb"); //si il deschid pt ca in el voi copia dupa datele trimise
                        free(name);
                        break;
                    case 'D': //DATA
                        length=r->payload[1]; //iau LEN ca sa stiu cat sa copiez la fiecare mesaj primit in DATA
                        memcpy(buffer, r->payload+4, length); //pun in buffer DATA
                        fwrite( buffer, 1, length, f); //scriu in fisier ce si cat trebuie 
                        break;
                    case 'Z': //END OF FILE
                        fclose(f); //inchid fisierul
                        break;

                }

                send_message(&t); //trimit mesajul
                printf("				[%d] ACK pentru [%d]\n ", t.payload[2], r->payload[2]);

                if(r->payload[3] == 'B') { //Daca e ACK, mesajul e trimis si pachetul e de tip EOT
                    return 0; //receiverul se termina
                }
                
            }
        }

        if(k==3){ //TIMEOUT de 3 ori consecutiv
	 		printf("3xTIMEOUT la [%s]\n", argv[0]);

            return -1;
        }

    }


	return 0;
}
