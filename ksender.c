#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10000

int main(int argc, char** argv) {
    msg t, *r;

    init(HOST, PORT);

    minikermit mk;
    sinit sdata;
    //initializez campurile structurilor
    memset(mk.DATA, 0, sizeof(mk.DATA));
    mk.SOH=0x01;
    mk.LEN=255;
    mk.SEQ=0;
    mk.TYPE = 'S';
    mk.MARK=0x0D;

    sdata.MAXL=250;
    sdata.TIME=5;
    sdata.NPAD=0x00;
    sdata.PADC=0x00;
    sdata.EOL=0x0D;
    sdata.QCTL=0;
    sdata.QBIN=0;
    sdata.CHKT=0;
    sdata.REPT=0;
    sdata.CAPA=0;
    sdata.R=0;

    memcpy(mk.DATA, &sdata, 11); //copiez structura sdata in DATA (cele 11 campuri)

    t.len=258;
    unsigned short crc =  crc16_ccitt(&mk, 254); //calculez crc
    int k, i;
    mk.CHECK=crc; //pun crc in campul CHECK

    memcpy(t.payload, &mk, 258); //copiez structura minikermit in payloadul mesajului dde trimis

	//pachetul SINIT
    k = 0;
    while(k<3){
        send_message(&t);
        printf("[%d] Send-init\n", t.payload[2]);
        r = receive_message_timeout(5000);
        if(r == NULL) { //Daca e timeout incrementez k
            k++;
        }
        else{
        	//k = 0;
            if(r->payload[3] != 'Y'){ //Daca e NACK
            	mk.SEQ=(mk.SEQ+1)%64;
                crc=crc16_ccitt(&mk, 254);
                mk.CHECK=crc;
                memcpy(t.payload, &mk, 258);
                k = 0; 
            }
            else k = 4; //DACA E ACK ies din while
        }
    }
    if(k==3){ //TIMEOUT 3 ori consecutiv 
    	printf("TIMEOUT la SEND INIT\n");
        return -1;
    }



   	//fisier, date, eof
    for(i=1; i<argc; i++){

    //  Pachete file header
        memset(mk.DATA, 0, 250);
        memcpy(mk.DATA, argv[i], strlen(argv[i])+1); //copiez numele fisierului
        mk.LEN=255;
        mk.SEQ=(mk.SEQ+1)%64;
        mk.TYPE='F'; //pun tipul
        crc =  crc16_ccitt(&mk, 254);
        mk.CHECK=crc; //pun crc
        memcpy(t.payload, &mk, 258); //copiez in payload mesajul

        k=0;
        while(k<3){
            send_message(&t);//il trimit
            printf("[%d] File Header %d\n", t.payload[2], i);
            r=receive_message_timeout(5000); //primesc raspuns
            if(r == NULL) {
                k++; //Cazul TIMEOUT
            }
            else if(r->payload[3] != 'Y'){ //daca primesc NACK voi trimite din nou mesaj cu un nou crc
                mk.SEQ=(mk.SEQ+1)%64;
                crc=crc16_ccitt(&mk, 254);
                mk.CHECK=crc;
                memcpy(t.payload, &mk, 258);
                k=0;
            }
            else k=4; //am primit ACK; ies din while
        }
        if(k==3){ //TIMEOUT 3 ori consecutiv
        	printf("TIMEOUT la FILE\n");
            return -1;
        }
        
        //Data din fisier
        FILE *f;
        int size;
        char buffer[251];

        f=fopen(argv[i], "rb");
        fseek(f, 0, SEEK_END);
        size = ftell(f); //m-am dus la sfarsit si calculez lungimea fisierului
        rewind(f); //ma intorc la inceputul fisierului
        size_t length; //de length ma voi ajuta sa stiu exact cat citesc din fisier
        int j=1;
        while(size > 0){
            memset(buffer, 0, sizeof(buffer)); //initializez bufferul cu 0 (pt siguranta)
            if(size>=250)
                length = fread(buffer, 1, 250, f);
            else
                length = fread(buffer, 1, size, f);
            mk.SEQ=(mk.SEQ+1)%64;
            mk.TYPE='D'; //tipul DATA
            memcpy(mk.DATA, buffer, length);
            size=size-length; //decrementez size cu cat citesc la fiecare mesaj pana ajunge la 0
            mk.LEN=length; //in LEN din structura pun length
            crc =  crc16_ccitt(&mk, 254);
            mk.CHECK=crc; //pun crc
            memcpy(t.payload, &mk, 258);

            k=0;
            while(k<3){ // Partea aceasta: ca la fisier(explicatii)
                send_message(&t);
                printf("[%d] Data %d\n", t.payload[2], j);
                memset(buffer, 0, sizeof(buffer));
                r=receive_message_timeout(5000);
                if(r == NULL) { //TIMEOUT
                    k++;
                }
                else if(r->payload[3] != 'Y'){ //NACK
                    mk.SEQ=(mk.SEQ+1)%64;
                    crc=crc16_ccitt(&mk, 254);
                    mk.CHECK=crc;
                    memcpy(t.payload, &mk, 258);
                    k=0;
                }
                else k=4; //ACK
            }
            if(k==3){ //TIMEOUT de 3 ori consecutiv
            	printf("TIMEOUT la DATA\n");
                return -1; //inchid sender
            }
            j++;

        }

    
    //  pachete end of file
        mk.TYPE='Z'; //Tipul EOF
        mk.SEQ=(mk.SEQ+1)%64;
        memset(mk.DATA, 0, 250); //DATA este goala
        crc =  crc16_ccitt(t.payload, t.len);
        mk.CHECK=crc; //pun crc
        fclose(f); //inchid fisierul din care am citit si trimis mesaje
        memcpy(t.payload, &mk, 258);

        k=0;
        while(k<3){ // Partea aceasta: ca la fisier(explicatii)
            send_message(&t);
            printf("[%d] EOF\n", t.payload[2]);
            r=receive_message_timeout(5000);
            if(r == NULL) { //TIMEOUT
                k++;
            }
            else if(r->payload[3] != 'Y'){ //NACK
            	k = 0;
                mk.SEQ=(mk.SEQ+1)%64;
                crc=crc16_ccitt(&mk, 254);
                mk.CHECK=crc;
                memcpy(t.payload, &mk, 258);
            }
            else k=4; //ACK
        }
        if(k==3){
            printf("TIMEOUT la EOF\n");
            return -1;
        }


    }


//  pachetul EOT
    mk.TYPE='B'; //tipul END OF TRANSACTION
    mk.SEQ=(mk.SEQ+1)%64;
    memset(mk.DATA, 0, 250);
    crc =  crc16_ccitt(&mk, 254);
    mk.CHECK=crc;
    memcpy(t.payload, &mk, 258);

    k=0;
    while(k<3){ // Partea aceasta: ca la fisier(explicatii)
        send_message(&t);
        printf("[%d] EOT\n", t.payload[2]);
        r=receive_message_timeout(5000);
        if(r==NULL) { //TIMEOUT
            k++;
        }
        else if(r->payload[3] != 'Y') { //NACK
            k = 0;
            mk.SEQ=(mk.SEQ+1)%64;
            crc=crc16_ccitt(&mk, 254);
            mk.CHECK=crc;
            memcpy(t.payload, &mk, 258);
        }
        else return 0; //Daca primesc ACK pt EOT totul se termina 
    }
    printf("TIMEOUT la EOT\n");
    return -1; //TIMEOUT de 3 ori consecutiv (ultima posibilitate)
}


