###############################################################################
########################### README - Tema_2 PCOM ##############################
###############################################################################

Implementarea server-ului se afla in fisierul server.c, iar cea a clientului de
tip TCP in fisierul subscriber.c. In helper.h se afla functiile ajutatoare si
structurile de date folosite in implementare. Anumite functii si structuri de
date au fost preluate din laboratorul de PCOM (Macro-ul de verificare erori,
schelet pentru functiile de recv_all/send_all si structuri). 

HELPERS.H
    -am folosit cate o structura pentru ficare tip de socket. Acestea contin
  topic, type si payload-ul mesajului. In plus, structura TCP contine IP-ul si
  portul de pe care mesajul este trimis, iar campul topic are dimensiuna 51 
  pentru terminatorul de sir.
    -structura client contine toate informatiile despre clientii conectati la
  server: id, socket, online(status-ul), un array de topics (structura cu sf-ul
  si titlul unui topic) si un array de structuri TCP, reprezentand mesajele
  primite de de client cat timp acesta a fost offline.
    -structura packet este cea care incapsuleaza cererile de la clientii TCP.
  Contine un camp de tip topic si un sir de caractere pentru request-ul trimis

SERVER.C
  Initial se se verifica numarul de argumente, se intializeaza, se seteaza 
  optiunile si se deschid socketii de tip TCP si UDP, se initializeaza tabela
  de file descriptori si se adauga socketii in aceasta. Serverul ruleaza
  intr-un loop infinit, care se opreste la modificarea flagului -server_online.
  In loop server-ul verifica tipul de 'mesaje' primite si rezolva cererile.
    - pentru un request de conexiune al unui client TCP, se verifica daca
    acesta exista in tabela de clienti. Daca nu exista, se adauga in tabela.
    In caz contrar in functie de status-ul clientului, acesta va fi trecut pe
    ONLINE si i se vor trimite mesajele primite cat timp a fost offline sau se
    va afisa un mesaj si se va inchide conexiunea daca clientul era deja online
    - pentru mesaje primite de la clientii UDP, se va completa structura TCP
    necesara pentru a trimite pachetul catre toti clientii abonati la topicul 
    respectiv. Datele din mesaje sunt convertite conform cerintei, iar packetul
    este trimis. Pentru clientii aflati in starea offline, packetele sunt 
    salvate in structura clientului.
    - pentru comenzi primite la STDIN, server-ul verifica daca este comanda
    exit. In caz afirmativ, se seteaza flag-ul server_online pe 0, ceea ce va
    opri loop-ul infinit si va inchide conexiunile cu toti clientii.
    - pentru requesturi de la clientii TCP, se verifica tipul de req. Pentru
    subscribe si unsubscribe se verifica daca clientul este abonat sau nu la
    topic si se procedeaza in consecinta (se adauga sau se elimina topicul din
    lista sa), iar pentru requestul 'exit' sau cazul in care clientul a fost
    oprit, se va inchide conexiunea cu aceasta, iar statusul va fi trecut pe 0.

SUBSCRIBER.C
  Similar se realizeaza toate initalizarile pentru deschiderea socket-ului de
  tip TCP, se realizeaza conexiunea cu server-ul, se trimtie id-ul clientului 
  pentru a fi adaugat in tabela de clienti a server-ului si se adauga socket-ul
  in tabela de file descriptori. Clientul ruleaza intr-un loop infinit, care 
  poate fi oprit doar prin 'exit'. In loop se pot trimite mesaje catre server
  in cazul in care se primeste o comanda de la STDIN sau se primesc mesaje de
  la server.
    - pentru comenzi de la STDIN, se verifica tipul de comanda si se 
    completeaza structura packet pentru a fi trimis un request catre server.
    In cazul in care se primeste comanda 'exit', se trimite requestul catre
    server iar loop-ul este intrerupt, urmand ca socket-ul sa fie inchis.
    - pentru packetele primite de la server, se va afisa mesajul conform
    cerintei din tema.

###############################################################################
Facultatea de Automatica si Calculatoare - Anul 2 2022-2023 - Grupa 322CCa
PETCU Andrei - PCOM - TEMA 2
###############################################################################