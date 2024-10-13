_______________________________________________________________________________________________________________________________
        SERVER_SIDE

    La deschidere, serverul va deschide un socket UDP pentru primirea mesajelor de la clientii udp, si un 
socket TCP care asculta pentru cereri de conexiune.
    Pentru retinera datelor fiecarui client care se conecteaza, este folosita o structura ce contine:
        -> file descriptorul pe care se ralizeaza comunicatia
        -> ID-ul sau
        -> un string cu toate topicurile la care a dat subscribe
        -> cate topicuri are in subscribe
        -> off-set-ul ultimului topic din stringul de subscribe

    La conectarea unui client, serverul primeste imediat ID-ul acestuia si verifica daca in vectorul de
clienti mai exista o structura cu acelasi id. Daca da, se verifica daca clientul cu ID-ul primti este
deconectat (campul de file descriptorul sau este setat pe -1). Daca acesta este conectat, conexiunea se inchide.
Daca nu este conectat, file descriptorul este updatat la noul file descriptor al conexiunii.
    La deconectare, structura asociata clientului cu file descriptorul care a primit 0 bytes este marcata
ca client deconectat(file descriptor setat pe -1).

    La primirea unui mesaj de subscribe de la un client, string-ul ce reprezinta numele topicului este copiat la
finalul stringului de subsciptions al clientului potrivit, cu tot cu NULL string terminator. Pentru unsubscribe,
numele topicului este suprascris de topicurile ce urmeaza in string-ul de topicuri.

    La primirea unui packet UDP, informatiile din acel pachet sunt copiate intr-o structura ce va fi transmisa
peste TCP. Aceasta structura contine IP-ul si port-ul sursei UDP, alaturi de ce contine pachetul UDP primit.
    Apoi, pentru fiercare client, se itereaza prin topicurile subscribed si se verifica daca acestea se potrivesc
cu topicul din pachetul UDP in felul urmator:
    -> se itereaza caracter cu caracter prin ambele string-uri
    -> daca topic-ul clientului contine '+' la caracterul curent, sari la urmatorul '/' in ambele string-uri
sau returneaza true daca se gaseste finalul string-ultimului
    -> daca topic-ul clientului contin '*' la caracterul curent, compara cuvantul ce il succede cu cuvintele
ce urmeaza in topic-ul din pachetul UDP; daca este gasit, se reia comparatia de la cuvintele gasite; daca nu,
returneaza fals. Daca dupa '*' nu urmeaza nimic, se returneaza true, intrucat acesta se poate potrivi cu orice si
oricate cuvinte urmeaza.
    Daca topic-urile se potrivesc, pachetul TCP este trimis la file descriptor-ul asociat clientului.

    La inchidera server-ului, toti file descriptor-ii deschisi sunt inchisi, inchizand astfel toate conexiunile.
_______________________________________________________________________________________________________________________________
        TCP CLIENT-SIDE

    La pornire, clientul deschide un socket TCP spre server si ii trmite ID-ul sau imediat.

    Cand clientul doreste sa dea subscribe la un topic, clientul trimite un string de forma "+<TOPIC>", respectiv
"-<TOPIC>" pentru unsubscribe.

    Cand un pachet TCP este primit, sectorul de data al acestuia este interpretat diferit in functie de tipul
mentionat in pachet.