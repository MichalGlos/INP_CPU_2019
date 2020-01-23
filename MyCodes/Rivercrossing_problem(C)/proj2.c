      
                //========================//
                //****  Michal Glos   ****//
                //**      xglosm01       **//
                //*         IOS          *//
                //**       Proj2        **//
                //****   28.4.2019    ****//
                //========================//

#define _POSIX_C_SOURCE 199309L // makro pro funkci usleep();
#define _XOPEN_SOURCE 500

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <semaphore.h>
#include <string.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h> 
#include <sys/mman.h> 
#include <sys/stat.h>
#include <fcntl.h>

#define SHM_NAME "/xglosm01_shm"
#define FILE_NAME "proj2.out"

typedef struct proc_sync
{
    int pier_h, pier_h_sailing, pier_h_sailed;                  // sdilene promenne
    int pier_s,pier_s_sailing, pier_s_sailed;
    int line_number;
    sem_t *write_data, *file_write;                             // semafory
    sem_t*pier_h_que, *pier_s_que, *pier_acces;
    sem_t *captain_sailing, *exit_boat, *sail;
} Proc_sync;

void p_wait_for_pier(int pier_wait_time, int id, char *name, int pier_capacity);                                            // funkce zamyka pristup na molo

void spawn_hackers(int hacker_spawn_time, int people_to_spawn, int sail_time, int pier_wait_time, int pier_capacity);       // pomocny proces pro generovani hackeru
void spawn_serfs(int serf_spawn_time, int people_to_spawn, int sail_time, int pier_wait_time, int pier_capacity);           // pomocny proces pro generovani sefu

void hacker_was_born(int id, int sail_time, int pier_wait_time, int pier_capacity);                                         // kod pro procesy hacker
void serf_was_born(int id, int sail_time, int pier_wait_time, int pier_capacity);                                           // kod pro procesy serf

void captain(int sail_time, int id, const char *name);                                                                      // funkce kapitana
void passenger(int id, const char *name);                                                                                   // funkce pasazera :)

void m_init();                                                                                                              // inicializace pameti
void m_clean();                                                                                                             // uklizeni pameti

void test_exit(int statement, const char *text);                                                                            // funkce ktera otestuje vyraz, if true -> tiskne error na stderr a exit(1)
void p_wait(int min_wait_time, int max_wait_time);                                                                                             // proces ceka nahodne dlouhou dobu v intervalu <0, max_wait_time>

void load_args(int argc, char *argv[], int *people_to_spawn, int *hacker_spawn_time, int *serf_spawn_time, int *sail_time, int *pier_wait_time, int *pier_capacity);    // funkce nacte argumenty

Proc_sync *sync_str;                                                                                                        // globalni promenne :  sdilena struktura pro synchronizaci procesu
FILE *action_log;                                                                                                           //                      file descriptor pro vystupni soubor
int shm_fd;                                                                                                                 //                      file descriptor pro sdilenou pamet

int main(int argc, char *argv[])
{
    pid_t pid;                                                                                                              // pid pro kontrolu identity procesu
    int people_to_spawn, hacker_spawn_time, serf_spawn_time, sail_time, pier_wait_time, pier_capacity;                      // deklarace promennych do kterych budou nacteny parametry programu                                                                                            
    load_args(argc, argv, &people_to_spawn, &hacker_spawn_time, &serf_spawn_time, &sail_time, &pier_wait_time, &pier_capacity);         // nacitani parametru programu
    m_init();                                                                                                               // inicializace pameti

    signal(SIGINT, m_clean);                                                                                                // po preruseni zvenci po sobe uklidi
    signal(SIGKILL, m_clean);
    signal(SIGQUIT, m_clean);
    
    pid = fork();                                                                                                           // fork - potomek pro generovani hackeru
    test_exit(pid < 0, "Doslo k chybe pri vytvareni potomka");                                                              // test uspesnosti forku

    if(pid == 0)                                                                                                            // potomek
        spawn_hackers(hacker_spawn_time, people_to_spawn, sail_time, pier_wait_time, pier_capacity);                        // funkce pro generovani hackeru
    else                                                                                                                    // rodic
    {
        pid = fork();                                                                                                       // fork pro proces generujici sefrs
        test_exit(pid < 0, "Doslo k chybe pri vytvareni potomka");                                                          // test uspesnosti forku
        if(pid == 0)                                                                                                        // potomek
            spawn_serfs(serf_spawn_time, people_to_spawn, sail_time, pier_wait_time, pier_capacity);                        // zacne spoustet procesy serf
        else
        {
            wait(NULL);                                                                                                     // rodic ceka na oba sve potomky
            wait(NULL);  
            m_clean();                                                                                                      // uklizeni pameti
        }
    }
}

void p_wait_for_pier(int pier_wait_time, int id, char *name, int pier_capacity)                         // funkce zamyka pristup na molo!!!
{
    sem_wait(sync_str->pier_acces);                                                                     // zamkne pristup na molo
    while(sync_str->pier_h + sync_str->pier_s >= pier_capacity)                                         // dokud je molo plne
    {
        sem_wait(sync_str->file_write);                                                                 // zapis procesu o opusteni fronty (molo je plne)
        fprintf(action_log,"%d: %s %d: leaves queue: %d: %d\n" ,++sync_str->line_number, name, id, sync_str->pier_h, sync_str->pier_s);
        fflush(action_log);
        sem_post(sync_str->file_write);

        sem_post(sync_str->pier_acces);                                                                 // odemkne pristup na molo
        p_wait(20, pier_wait_time);
        sem_wait(sync_str->file_write);                                                                 // zapis procesu o probuzeni
        fprintf(action_log,"%d: %s %d: is back\n" ,++sync_str->line_number, name, id);
        fflush(action_log);
        sem_post(sync_str->file_write);
        sem_wait(sync_str->pier_acces);                                                                 // pocka na pristup na molo, aby otestoval volne misto
    }
}

void spawn_hackers(int hacker_spawn_time, int people_to_spawn, int sail_time, int pier_wait_time, int pier_capacity)
{   
    int id = 0;                                                         // nesdilena promenna id. Parent Generujici procesy ma id 0, dale se s kazdym dalsim ditetem id inkrementuje
    pid_t pid;                                                              
    srand(time(NULL) % (int)getpid());                                  // seed random, getpid pro rozdilny seed pro oba procesy generujici serf a hackers

    for(int i = 0; i < people_to_spawn; i++)
    {
        p_wait(0, hacker_spawn_time);                                      // uspani procesu, pauza pred vygenerovanim noveho hackera
        pid = fork();                                                   // vygenerovan novy hacker
        test_exit(pid < 0, "Doslo k chybe pri vytvareni potomka");          
        if(pid == 0)                                                    // potomek
        {
            id = i + 1;                                                 // potomkovi je prirazeno id a opusti cyklus generovani potomku
            break;
        }
    }
    if(pid == 0)                                                        // potomek
        hacker_was_born(id, sail_time, pier_wait_time, pier_capacity);  // potomek zacne svou funkci
    else
    {
        for(int i = 0; i < people_to_spawn; i++)                        // rodic ceka na vsechny sve potomky
            wait(NULL);
        exit(0);
    }
}

void spawn_serfs(int serf_spawn_time, int people_to_spawn, int sail_time, int pier_wait_time, int pier_capacity)
{   
    int id = 0;                                                         // nesdilena promenna id. Parent Generujici procesy ma id 0, dale se s kazdym dalsim ditetem id inkrementuje
    pid_t pid;
    srand(time(NULL) % (int)getpid());                                  // seed random, getpid pro rozdilny seed pro oba procesy generujici serf a hackers

    for(int i = 0; i < people_to_spawn; i++)                            // for cyklus generujici potomky
    {
        p_wait(0, serf_spawn_time);                                        // uspani procesu, pauza ored vygenerovanim noveho serfa
        pid = fork();
        test_exit(pid < 0, "Doslo k chybe pri vytvareni potomka");
        if(pid == 0)                                                    // potomek pokracuje dal do kodu, rodic pokracuje v generovani dalsich potomku
        {
            id = i + 1;                                                 // potomkovi je prirazeno id a opusti cyklus
            break;
        }
    }
    if(pid == 0)
        serf_was_born(id, sail_time, pier_wait_time, pier_capacity);    // potomek skace na tuto funkci
    else
    {
        for(int i = 0; i < people_to_spawn; i++)                        // Cekani na vsechny sve potomky
            wait(NULL);
        exit(0);
    }
}

void hacker_was_born(int id, int sail_time, int pier_wait_time, int pier_capacity)
{
    sem_wait(sync_str->file_write);                                                 // zapis o narozeni hackera
    fprintf(action_log,"%d: HACK %d: starts\n" ,++sync_str->line_number, id);
    fflush(action_log);
    sem_post(sync_str->file_write);

    p_wait_for_pier(pier_wait_time, id, "HACK", pier_capacity);                     // proces drive nebo pozdeji vstoupi na molo

    sem_wait(sync_str->file_write);                                                 // zapis do souboru o vstupu na molo, zamek na data, inkrementuje se pocek hackeru na molu
    ++sync_str->pier_h;                                                             // na molo prisel hacker
    fprintf(action_log,"%d: HACK %d: waits: %d: %d\n" ,++sync_str->line_number, id, sync_str->pier_h, sync_str->pier_s);
    fflush(action_log);
    sem_post(sync_str->file_write);   
    sem_post(sync_str->pier_acces);                                                 // povoli se pristup na molo

    sem_wait(sync_str->write_data);                                                 //kriticka sekce - pocitani lidi na molu a rozhodnuti o roli procesu (kapitan / pasazer)
    sync_str->pier_h_sailing++;
    if(sync_str->pier_h_sailing - sync_str->pier_h_sailed == 4)                     // kapitan 
    {
        sync_str->pier_h_sailed += 4;                                               // procesy ziskaly sveho kapitana, nemaji jeste lod
        sem_post(sync_str->write_data);  
        
        sem_wait(sync_str->captain_sailing);                                        // zamkne se lod
        sem_post(sync_str->pier_h_que);                                             // nalodeni dalsich 3 pasazeru
        sem_post(sync_str->pier_h_que);
        sem_post(sync_str->pier_h_que);
        
        sem_wait(sync_str->pier_acces);
        sem_wait(sync_str->file_write);                                             // zapisuje se kapitanovo nalodeni a odectou se 4 lidi z mola
        sync_str->pier_h -= 4;
        fprintf(action_log, "%d: HACK %d: boards: %d: %d\n", ++sync_str->line_number, id, sync_str->pier_h, sync_str->pier_s);
        fflush(action_log);
        sem_post(sync_str->file_write);
        sem_post(sync_str->pier_acces);
        captain(sail_time, id, "HACK");                                             // kapitan se vyda na more
    }
    else if(sync_str->pier_h_sailing - sync_str->pier_h_sailed == 2 && sync_str->pier_s_sailing - sync_str->pier_s_sailed >= 2)     // kapitan
    {
        sync_str->pier_h_sailed += 2;                                               // procesy ziskalu noveho kapitana, nemaji jeste lod
        sync_str->pier_s_sailed += 2;
        sem_post(sync_str->write_data); 

        sem_wait(sync_str->captain_sailing);                                        // zamknuti lodi
        sem_post(sync_str->pier_h_que);                                             // nalodeni zbytku posadky
        sem_post(sync_str->pier_s_que);
        sem_post(sync_str->pier_s_que);

        sem_wait(sync_str->pier_acces);
        sem_wait(sync_str->file_write);                                             // zapisuje se kapitanovo nalodeni
        sync_str->pier_h -= 2;                                                      // odectou se 4 lidi z mola
        sync_str->pier_s -= 2;
        fprintf(action_log, "%d: HACK %d: boards: %d: %d\n", ++sync_str->line_number, id, sync_str->pier_h, sync_str->pier_s);
        fflush(action_log);
        sem_post(sync_str->file_write); 
        sem_post(sync_str->pier_acces);
        captain(sail_time, id, "HACK");                                             // volani fuknce kapitana
    }
    else
    {
        sem_post(sync_str->write_data);                                 
        sem_wait(sync_str->pier_h_que);                                             // cekani pasazera na nalodeni
        passenger(id, "HACK");                                                      // zavolani funkce pasazera
    }    
}

void serf_was_born(int id, int sail_time, int pier_wait_time, int pier_capacity)
{
    sem_wait(sync_str->file_write);                                                 // zapis o narozeni serfa
    fprintf(action_log,"%d: SERF %d: starts\n" ,++sync_str->line_number, id);
    fflush(action_log);
    sem_post(sync_str->file_write);

    p_wait_for_pier(pier_wait_time, id, "SERF", pier_capacity);                    // cekani na molo

    sem_wait(sync_str->file_write);                                                 // zapis do souboru o vstupu na molo, zamek na data, inkrementuje se pocek serfu na molu
    sync_str->pier_s++;                                                             // na molo prisel serf
    fprintf(action_log,"%d: SERF %d: waits: %d: %d\n" ,++sync_str->line_number, id, sync_str->pier_h, sync_str->pier_s);
    fflush(action_log);
    sem_post(sync_str->file_write);
    sem_post(sync_str->pier_acces);

    sem_wait(sync_str->write_data);                                                 //kriticka sekce - pocitani lidi na molu a rozhodnuti o roli procesu (kapitan / pasazer)
    sync_str->pier_s_sailing++;
    if(sync_str->pier_s_sailing - sync_str->pier_s_sailed == 4)                     // kapitan
    {
        sync_str->pier_s_sailed += 4;                                               // procesy ziskalu noveho kapitana, nemaji jeste lod
        sem_post(sync_str->write_data); 

        sem_wait(sync_str->captain_sailing);                                        // zamkne se lod
        sem_post(sync_str->pier_s_que);                                             // nalodi se dalsi pasazeri
        sem_post(sync_str->pier_s_que);
        sem_post(sync_str->pier_s_que);

        sem_wait(sync_str->pier_acces);
        sem_wait(sync_str->file_write);                                             // zapis nalodeni kapitana
        sync_str->pier_s -= 4;                                                      // odetou se 4 lidi z mola
        fprintf(action_log, "%d: SERF %d: boards: %d: %d\n", ++sync_str->line_number, id, sync_str->pier_h, sync_str->pier_s);
        fflush(action_log);
        sem_post(sync_str->file_write); 
        sem_post(sync_str->pier_acces);
        captain(sail_time, id, "SERF");                                             // fuknce pro kapitana
    }
    else if(sync_str->pier_s_sailing - sync_str->pier_s_sailed == 2 && sync_str->pier_h_sailing - sync_str->pier_h_sailed >= 2)     // kapitan
    {
        sync_str->pier_s_sailed += 2;                                               // procesy ziskalu noveho kapitana, nemaji jeste lod
        sync_str->pier_h_sailed += 2;
        sem_post(sync_str->write_data);

        sem_wait(sync_str->captain_sailing);                                        // zamkne se lod
        sem_post(sync_str->pier_h_que);                                             // nalodi se dalsi pasazeri
        sem_post(sync_str->pier_h_que);
        sem_post(sync_str->pier_s_que);
        
        sem_wait(sync_str->pier_acces);
        sem_wait(sync_str->file_write);                                             // zapis kapitana                             
        sync_str->pier_s -= 2;                                                      // odectou se 4 pasazeri
        sync_str->pier_h -= 2;
        fprintf(action_log, "%d: SERF %d: boards: %d: %d\n", ++sync_str->line_number, id, sync_str->pier_h, sync_str->pier_s);
        fflush(action_log);
        sem_post(sync_str->file_write);  
        sem_post(sync_str->pier_acces);
        captain(sail_time, id, "SERF");                                             // volani funkce kapitana
    }
    else
    {
        sem_post(sync_str->write_data);
        sem_wait(sync_str->pier_s_que);                                             // cekani na molu
        passenger(id, "SERF");                                                      // nalodeni pasazera
    }
}

void captain(int sail_time, int id, const char *name)
{
    p_wait(0, sail_time);                                                              // proces ceka random dobu (doba plavby)
    sem_post(sync_str->sail);                                                       // plavba byla dokoncena, oznamuje pasazerum
    sem_post(sync_str->sail);
    sem_post(sync_str->sail);
    
    sem_wait(sync_str->exit_boat);                                                  // ceka az pasazeri opusti lod aby mohl vystoupit
    sem_wait(sync_str->exit_boat);
    sem_wait(sync_str->exit_boat);
    
    sem_wait(sync_str->pier_acces);
    sem_wait(sync_str->file_write);                                                 // kapitan zapisuje vystup z lodi
    fprintf(action_log, "%d: %s %d: captain exits: %d: %d\n", ++sync_str->line_number, name, id, sync_str->pier_h, sync_str->pier_s);
    fflush(action_log);
    sem_post(sync_str->file_write);
    sem_post(sync_str->pier_acces);
    sem_post(sync_str->captain_sailing);                                            // kapitan se vzdava lodi
}

void passenger(int id, const char *name)
{
    sem_wait(sync_str->sail);                                                       // je na palube, ceka na dapluti
    sem_wait(sync_str->pier_acces);                                                 // pasazer zapisuje vystup lodi
    sem_wait(sync_str->file_write);
    sem_post(sync_str->exit_boat);                                                  // pasazer vystupuje z lodi
    fprintf(action_log, "%d: %s %d: member exits: %d: %d\n", ++sync_str->line_number, name, id, sync_str->pier_h, sync_str->pier_s);
    fflush(action_log);
    sem_post(sync_str->file_write);
    sem_post(sync_str->pier_acces);
}

void m_init()
{
    shm_fd = shm_open(SHM_NAME, O_RDWR|O_CREAT, 0644);                             // inicializace sdilene pameti
    test_exit((shm_fd < 0), "Nastala chyba pri otevirani sdilene pameti\n");

    int eflag = ftruncate(shm_fd, sizeof(Proc_sync));                               // formatovani pameti
    test_exit((eflag != 0), "Chyba pri inicializaci sdilene pameti\n");

    sync_str = mmap(NULL, sizeof(Proc_sync), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);    // mapovani casti pameti pro sdilene procesy
    test_exit(sync_str == MAP_FAILED, "Chyba pri mapovani pameti do promenne\n");

    sync_str->pier_h = 0;
    sync_str->pier_h_sailing = 0;                                                   // inicializace sdilenych promennych
    sync_str->pier_h_sailed = 0;
    sync_str->pier_s = 0;
    sync_str->pier_s_sailing = 0;
    sync_str->pier_s_sailed = 0;
    sync_str->line_number = 0;
                                                                                    // inicializace semaforu
    sync_str->write_data = sem_open("/xglosm01_sem1", O_CREAT | O_EXCL, 0644, 1);   
    sync_str->pier_h_que = sem_open("/xglosm01_sem2", O_CREAT | O_EXCL, 0644, 0);   
    sync_str->pier_s_que = sem_open("/xglosm01_sem3", O_CREAT | O_EXCL, 0644, 0);   
    sync_str->captain_sailing = sem_open("/xglosm01_sem4", O_CREAT | O_EXCL, 0644, 1);   
    sync_str->exit_boat = sem_open("/xglosm01_sem5", O_CREAT | O_EXCL, 0644, 0);   
    sync_str->sail = sem_open("/xglosm01_sem6", O_CREAT | O_EXCL, 0644, 0);   
    sync_str->file_write = sem_open("/xglosm01_sem7", O_CREAT | O_EXCL, 0644, 1);   
    sync_str->pier_acces = sem_open("/xglosm01_sem8", O_CREAT | O_EXCL, 0644, 1);  
                                                                                    // test validity semaforu
    test_exit(sync_str->write_data == SEM_FAILED, "Chyba pri otevirani semaforu\n");
    test_exit(sync_str->pier_h_que == SEM_FAILED, "Chyba pri otevirani semaforu\n");
    test_exit(sync_str->pier_s_que == SEM_FAILED, "Chyba pri otevirani semaforu\n");
    test_exit(sync_str->captain_sailing == SEM_FAILED, "Chyba pri otevirani semaforu\n");    
    test_exit(sync_str->exit_boat == SEM_FAILED, "Chyba pri otevirani semaforu\n");
    test_exit(sync_str->sail == SEM_FAILED, "Chyba pri otevirani semaforu\n");
    test_exit(sync_str->file_write == SEM_FAILED, "Chyba pri otevirani semaforu\n");
    test_exit(sync_str->pier_acces == SEM_FAILED, "Chyba pri otevirani semaforu\n");

    action_log = fopen(FILE_NAME,"w");                                          // otevreni souboru pro zapis
    test_exit(action_log == NULL, "Chyba pri otevirani souboru\n");
    setbuf(action_log, NULL);
}

void m_clean()
{
    sem_close(sync_str->write_data);                                            // zavirani semaforu
    sem_close(sync_str->pier_s_que);
    sem_close(sync_str->pier_h_que);
    sem_close(sync_str->captain_sailing);
    sem_close(sync_str->exit_boat);
    sem_close(sync_str->sail);
    sem_close(sync_str->file_write);
    sem_close(sync_str->pier_acces);

    sem_unlink("/xglosm01_sem1");                                               // smazani semaforu
    sem_unlink("/xglosm01_sem2");
    sem_unlink("/xglosm01_sem3");
    sem_unlink("/xglosm01_sem4");
    sem_unlink("/xglosm01_sem5");
    sem_unlink("/xglosm01_sem6");
    sem_unlink("/xglosm01_sem7");
    sem_unlink("/xglosm01_sem8");

    munmap(sync_str, sizeof(Proc_sync));                                        // odmapovani sdilene pameti

    shm_unlink(SHM_NAME);                                                       // smazani sdilene pameti

    fclose(action_log);                                                         // zavreni file descriptoru pro zapis procesu
}

void test_exit(int statement, const char *text)
{
    if(statement)                                                               // if statement true -> exits and prints error to stderr
    {
        fprintf(stderr, "%s", text);
        exit(1);
    }
}

void p_wait(int min_wait_time, int max_wait_time)
{
    if(min_wait_time < max_wait_time)                                                      // uspani procesu na dobu v intervalu <0, max_wait_time>
    {
        unsigned int wait_time = rand() % (max_wait_time - min_wait_time) + min_wait_time + 1;
        usleep(wait_time * 1000);
    }
}

void load_args(int argc, char *argv[], int *people_to_spawn, int *hacker_spawn_time, int *serf_spawn_time, int *sail_time, int *pier_wait_time, int *pier_capacity)
{
    test_exit(argc != 7, "Program ocekava 6 argumentu");                        // kontrola spravneho poctu argumentu

    for(int i = 1; i < argc; i++)                                               // kontrola jestli vsechny argumenty obsahuji pouze cislice
    {
        for(int j = 0; argv[i][j] != '\0'; j++)
            test_exit(argv[i][j] < '0' || argv[i][j] > '9', "Program prijma jako argumenty pouze cela cisla");
    }

    *people_to_spawn = atoi(argv[1]);                                           // nacitani jednotlivych parametru a testovani jejich validity
    test_exit(*people_to_spawn < 2 || *people_to_spawn % 2 == 1, "Prvni argument musi byt cele sude cislo vetsi nebo rovno 2");
        
    *hacker_spawn_time = atoi(argv[2]);
    test_exit(*hacker_spawn_time < 0 || *hacker_spawn_time > 2000, "Druhy argument musi byt cele cislo v intervalu <0, 2000>");

    *serf_spawn_time = atoi(argv[3]);
    test_exit(*serf_spawn_time < 0 || *serf_spawn_time > 2000, "Treti argument musi byt cele cislo v intervalu <0, 2000>");

    *sail_time = atoi(argv[4]);
    test_exit(*sail_time < 0 || *sail_time > 2000, "Ctvrty argument musi byt cele cislo v intervalu <0, 2000>");

    *pier_wait_time = atoi(argv[5]);
    test_exit(*pier_wait_time < 20 || *pier_wait_time > 2000, "Paty argument musi byt cele cislo v intervalu <0, 2000>");

    *pier_capacity = atoi(argv[6]);
    test_exit(*pier_capacity < 5, "Sesty argument musi bt cele cislo vetsi nebo rovno 5");
}