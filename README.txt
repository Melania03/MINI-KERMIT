	Am creeat doua structuri: una minikermit, pe care o voi copia mereu in payloadul mesajului, care contine campurile SOH, LEN, SEQ, TYPE, CHECK si MARK si a doua sinit, care va fi copiata in campul DATA pentru mesajele pachetului Send Init.

	In Sender:
	Pentru Send Init, trimit mesajul (incep cu seq=0, type S, calculez crc si il pun in check, lenul e 255 la toate pachetele, exceptand data), primesc raspunsul, si in functie de el astept, mai trimit mesajul inca o data sau trec la urmatorul pachet. Daca e timeout, incrementez un k, trimit iar mesaj si in cazul in care k va ajunge 3 (3 timeouturi) senderul se va opri. Daca primesc NACK, trimit din nou mesajul, cu seq incrementat si un nou crc, iar daca primesc ACK trec la urmatorul pachet.
	Pentru File Headere, Data si EOF, am folosit un for de la 1 pana la argc. Prima data trimit mesaj cu data numele fisierului, type F, check crc, seq incrementat. Primesc mesaj. Verific daca e timeout, daca e NACK sau ACK. Analog Send Init.
	Pentru Data, aflu mai intai lungimea fisierului cu ftell, pentru ca din ea o sa decrementez lungimea pe care o citesc (cu fread) din datele fisierului la fiecare mesaj. Cat timp lungimea e mai mare decat 0, citesc din fisier si trimit mesaje cu data, bucati de 250 de bytes, si ultima (posibil) mai mica de atat (Exemplu fisier de 505 bytes: 250 - 250 - 5). Trimit un mesaj, astept raspuns, si in functie de acesta vad ce fac. Analog File Header. De data aceasta ma folosesc si de campul LEN din structura pt ca voi pune lungimea pe care o citesc, ca receiverul sa stie cat scrie in fisierul care se creeaza.
	Dupa ce am trimis File Header, Datele din acel fisier, trimit EOF, unde am folosit acelasi rationament. Trimit mesaj si in functie de ceea ce primesc, astept, mai trimit sau trec la urmatorul pachet.
	De asemenea, EOT este la fel gandit, doar ca daca primesc ACK, senderul se va termina, return 0.

	In Receiver:
	Pentru Send Init, primescc mesajul, daca e timeout, incrementez k, iar daca nu e timeout, calculez crc-ul mesajului primit si il verific cu crc-ul din check. Daca nu sunt la fel, mai primesc acelasi pachet pana cand sunt egale si e bine, iar daca sunt, trimit acelasi mesaj, dar cu tipul Y, ACK inapoi la sender.
	Pentru celelalte pachete, folosesc o bucla infinita, pentru ca nu stiu cate mesaje primesc. Primesc cate un mesaj si verific. Daca e timeout, doar incrementez un k si astept, pt ca daca intarzie mesajul pana la receiver, trece timp si in sender, si asa acesta din urma va mai  trimite o data acelasi pachet. Daca nu e timeout, calculez crc-ul si il verific cu cel din check. Daca nu sunt la fel, trimit mesaj cu NACK, daca sunt, switch pe cazuri si trimit mesaj cu ACK.
	Cazul F, creez fisierul cu numele recv_ + numele pe care il primesc prin campul data. Deschid fisierul unde urmeaza sa scriu.
	Cazul D, copiez intr-un buffer tot ce primesc prin data (bucati de continut din fisier cu lungimea din len) si dupa pun ce e in buffer in fisier, imi scriu noul fisier.
	Cazul Z, (End of File), doar inchid fisierul.
	Daca mesajul a fost corect, trimit ACK, iar la sfarsit verific daca tipul mesajului corect a fost B, End of Transaction, deoarece atunci receiverul se va inchide. 
	La timeout aceeasi poveste, daca se primesc 3 mesaje intarziate, k-ul ajunge 3 si receiverul se inchide, return -1.


Georgescu Melania, 322CC



