#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define SIZE_OF_SHORT_STRING    64
#define SIZE_OF_LONG_STRING     512
#define SIZE_OF_4_CHAR          (sizeof(char)*4)
#define SIZE_OF_INT             (sizeof(int))
#define SIZE_OF_MAIL            (sizeof(mail_t))
#define SIZE_OF_MAIL_BOX        (sizeof(mailbox))
#define BOX_SIZE                5
#define CLIENT_NUMBER           10

#define JOIN        3
#define BROADCAST   4
#define LIST        5
#define WHISPER     6
#define LEAVE       7

#define SUCCESS  0
#define FAILURE -1
#define TRUE     1
#define FALSE    0


typedef struct __MAIL {
    int from;
    int type;
    char sstr[SIZE_OF_SHORT_STRING];
    char lstr[SIZE_OF_LONG_STRING];
} mail_t;


typedef struct __MAILBOX {
    int fd;
    void *box_addr;
} mailbox;

typedef void *mailbox_t;

/*-----------------------------------------------*/
/*--------------     mailbox API    -------------*/
/*-----------------------------------------------*/
mailbox_t mailbox_open(int id)
{

    mailbox_t mt;
    int flag = 0;
    char head[4] = "0";
    char tail[4] = "0";

    //shared memory object's name
    char boxName[20] = "__mailbox_";
    char mailBoxId[5];
    int fd;
    sprintf(mailBoxId,"%d",id);
    strcat(boxName,mailBoxId);

    mailbox *mb = (mailbox*)malloc(SIZE_OF_MAIL_BOX);
    mt = mb;
    char address[20] = "/dev/shm/";
    strcat(address,boxName);

    if(access(address,F_OK) != 0) {
        printf("first time build: ");
        printf("%s.\n",address);
        fd = shm_open(boxName,O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        mb->fd = fd;
        ftruncate(fd, SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
        printf("mailbox.size = %d\n",SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
        mb->box_addr = mmap(NULL,SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2,
                            PROT_READ | PROT_WRITE,
                            MAP_SHARED, fd, 0);
        //mb->in = 0;
        if(mb->box_addr == MAP_FAILED) {
            printf("mapping failed");
            return NULL;
        }
        memcpy(mb->box_addr, head, SIZE_OF_4_CHAR);
        memcpy(mb->box_addr + SIZE_OF_4_CHAR, tail, SIZE_OF_4_CHAR);
        return mt;
    }

    mailbox *m = (mailbox*)mt;
    fd = shm_open(boxName,O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    m->fd = fd;
    ftruncate(fd, SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
    printf("mailbox.size = %d\n",SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
    m->box_addr = mmap(NULL,SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2,
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED, fd, 0);

    //set -> shared memory.size = mailbox.size
    if(m->box_addr == MAP_FAILED) {
        printf("mapping failed");
        return NULL;
    }
    //printf("Map addr is 0x%08x\n",m->box_addr);


    return mt;
}


int mailbox_unlink(int id)
{

    char boxName[20] = "__mailbox_";
    char mailBoxId[5];
    sprintf(mailBoxId,"%d",id);
    strcat(boxName,mailBoxId);
    int s = shm_unlink(boxName);
    if(s == 0) {
        printf("unlink successfully.\n");
        return SUCCESS;
    } else if(s < 0) {
        printf("unlink failed.\n");
        return FAILURE;
    }

}

int mailbox_close(mailbox_t box)
{
    mailbox *t = (mailbox*) box;
    int f = munmap(t->box_addr, SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
    int s = close(t->fd);
    if(s == 0) {
        printf("close successfully.\n");
        return SUCCESS;
    } else {
        printf("close failed.\n");
        return FAILURE;
    }

}


int mailbox_send(mailbox_t box, mail_t *mail)
{
    if(mailbox_check_full(box) == TRUE)
        return FAILURE;

    mailbox *m = (mailbox*)box;
    char head[4];
    memmove(head,m->box_addr,SIZE_OF_4_CHAR);
    int countMail = atoi(head);
    printf("send-head=%d\n",countMail);
    char from[4];
    char type[4];
    sprintf(from,"%d",mail->from);
    sprintf(type,"%d",mail->type);

    //write mail
    memmove(m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL,
            from,SIZE_OF_INT);
    memmove(m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL + SIZE_OF_INT,
            type,SIZE_OF_INT);
    memmove(m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL + SIZE_OF_INT*2,
            mail->sstr,SIZE_OF_SHORT_STRING);
    memmove(m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL + SIZE_OF_INT*2 + SIZE_OF_SHORT_STRING,
            mail->lstr,SIZE_OF_LONG_STRING);

    countMail = (countMail + 1) % BOX_SIZE;
    sprintf(head,"%d",countMail);
    memmove(m->box_addr,head,SIZE_OF_4_CHAR);

    return SUCCESS;
}


int mailbox_recv(mailbox_t box, mail_t *mail)
{
    if (mailbox_check_empty(box) == TRUE)
        return FAILURE;

    mailbox *m = (mailbox*)box;

    char tail[4];
    memcpy(tail, m->box_addr + SIZE_OF_4_CHAR,SIZE_OF_4_CHAR);
    int countMail = atoi(tail);
    char from[4];
    char type[4];

    memcpy(from, m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL, SIZE_OF_INT);
    memcpy(type, m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL + SIZE_OF_INT, SIZE_OF_INT);
    memcpy(mail->sstr, m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL + SIZE_OF_INT +
           SIZE_OF_INT, SIZE_OF_SHORT_STRING);
    memcpy(mail->lstr, m->box_addr + SIZE_OF_4_CHAR*2 + countMail*SIZE_OF_MAIL + SIZE_OF_INT +
           SIZE_OF_INT + SIZE_OF_SHORT_STRING, SIZE_OF_LONG_STRING);

    mail->from = atoi(from);
    mail->type = atoi(type);

    countMail = (countMail + 1) % BOX_SIZE;
    sprintf(tail,"%d",countMail);
    memmove(m->box_addr + SIZE_OF_4_CHAR,tail,SIZE_OF_4_CHAR);

    return SUCCESS;
}

int mailbox_check_empty(mailbox_t box)
{
    //empty -> return true;
    //not empty -> return false;
    mailbox *m = (mailbox*)box;
    char head[4];
    char tail[4];
    memcpy(head,m->box_addr,SIZE_OF_4_CHAR);
    memcpy(tail,m->box_addr + SIZE_OF_4_CHAR,SIZE_OF_4_CHAR);
    int h = atoi(head);
    int t = atoi(tail);

    if( h == t)
        return TRUE;
    else
        return FALSE;
}

int mailbox_check_full(mailbox_t box)
{
    //full ->return true;
    // not full -> return false;
    mailbox *m = (mailbox*)box;
    char head[4];
    char tail[4];
    memcpy(head,m->box_addr,SIZE_OF_4_CHAR);
    memcpy(tail,m->box_addr + SIZE_OF_4_CHAR,SIZE_OF_4_CHAR);

    int h = atoi(head);
    int t = atoi(tail);

    if( (h + 1)% BOX_SIZE == t )
        return TRUE;
    else
        return FALSE;
}

/*-----------------------------------------------*/

int inRoom(int id[], int c, int d)
{
    for(int i = 0; i < c; i++) {
        if(id[i] == d)
            return TRUE;
    }
    return FALSE;
}



/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

int main()
{

    mailbox_t server_t = mailbox_open(0);


    // store member in charroom1 and chatroom2
    char name1[CLIENT_NUMBER][16];
    int id1[CLIENT_NUMBER];
    int cnum1 = 0;

    char name2[CLIENT_NUMBER][16];
    int id2[CLIENT_NUMBER];
    int cnum2 = 0;

    //non-blocking I/O
    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    char buf[SIZE_OF_LONG_STRING];
    memset(buf, 0, sizeof(buf));
    int nread;
    fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);

    while(TRUE) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO,&fds);
        select(STDERR_FILENO+1, &fds,NULL, NULL, &tv);

        if(FD_ISSET(0,&fds)){
            ioctl(0,FIONREAD, &nread);
            nread = read(0, buf, nread);
            buf[nread] = 0;
            if(strncmp(buf,"CLOSE",5))
                break;
        }


        //server deal with mails
        mail_t smp;
        if(mailbox_recv(server_t,&smp) == SUCCESS) {
            printf("recv- from=%d  type=%d  sstr=%s   lstr=%s",smp.from,smp.type, smp.sstr, smp.lstr);

            if(smp.type == JOIN) {
                if(strcmp(smp.lstr, "1") == 0) {
                    sprintf(name1[cnum1],"%s",smp.sstr);
                    id1[cnum1] = smp.from;
                    cnum1 += 1;

                    mail_t m ;
                    m.from = 0;
                    m.type = JOIN;
                    strcpy(m.sstr,name1[cnum1 - 1]);

                    printf("join - %s\n",m.sstr);
                    //broadcast
                    for(int k = 0; k < cnum1; k++) {
                        mailbox_t mt = mailbox_open(id1[k]);
                        mailbox_send(mt,&m);
                        //mailbox_close(id[k]);
                    }
                } else if(strcmp(smp.lstr, "2") == 0) {
                    sprintf(name2[cnum2],"%s",smp.sstr);
                    id2[cnum2] = smp.from;
                    cnum2 += 1;

                    mail_t m ;
                    m.from = 0;
                    m.type = JOIN;
                    strcpy(m.sstr,name2[cnum2 - 1]);

                    printf("join - %s\n",m.sstr);
                    //broadcast
                    for(int k = 0; k < cnum2; k++) {
                        mailbox_t mt = mailbox_open(id2[k]);
                        mailbox_send(mt,&m);
                        //mailbox_close(id[k]);
                    }

                }


            } else if(smp.type == LEAVE) {

                if(inRoom(id1, cnum1, smp.from)) {
                    for(int i = 0; i < cnum1; i++) {
                        if (id1[i] == smp.from) {
                            int j;
                            for(j = i; j < cnum1 - 1; j++) {
                                id1[j] = id1[j + 1];
                                strcpy(name1[j],name1[j + 1]);
                            }
                            id1[j] = 0;
                        }
                    }
                    cnum1 -= 1;
                } else if(inRoom(id2, cnum2, smp.from)) {

                    for(int i = 0; i < cnum2; i++) {
                        if (id2[i] == smp.from) {
                            int j;
                            for(j = i; j < cnum2 - 1; j++) {
                                id2[j] = id2[j + 1];
                                strcpy(name2[j],name2[j + 1]);
                            }
                            id2[j] = 0;
                        }
                    }
                    cnum2 -= 1;
                }

            } else if(smp.type == BROADCAST) {
                mail_t m ;
                m.from = 0;
                m.type = BROADCAST;
                //m.sstr = //name;


                if(inRoom(id1, cnum1, smp.from)) {
                    int k;
                    for(k = 0; k < cnum1; k++) {
                        if(id1[k] == smp.from)
                            break;
                    }

                    strcpy(m.sstr,name1[k]);
                    strcpy(m.lstr,smp.lstr);

                    printf("gonna broadcast: from=%d  sstr=%s  lstr=%s",m.from,m.sstr,m.lstr);
                    //broadcast
                    for(k = 0; k < cnum1; k++) {
                        if(id1[k] != smp.from) {
                            mailbox_t mt = mailbox_open(id1[k]);
                            mailbox_send(mt,&m);
                        }
                    }

                } else if(inRoom(id2, cnum2, smp.from)) {
                    int k;
                    for(k = 0; k < cnum2; k++) {
                        if(id2[k] == smp.from)
                            break;
                    }

                    strcpy(m.sstr,name2[k]);
                    strcpy(m.lstr,smp.lstr);

                    printf("gonna broadcast: from=%d  sstr=%s  lstr=%s",m.from,m.sstr,m.lstr);
                    //broadcast
                    for(k = 0; k < cnum2; k++) {
                        if(id2[k] != smp.from) {
                            mailbox_t mt = mailbox_open(id2[k]);
                            mailbox_send(mt,&m);
                        }
                    }

                }


            } else if(smp.type == LIST) {
                char namestr[16];

                mail_t listmail;
                listmail.from = 0;
                listmail.type = LIST;

                if(inRoom(id1, cnum1, smp.from)) {
                    char space[3] = "; ";

                    strcpy(listmail.lstr, name1[0]);
                    strcat(listmail.lstr, space);
                    for(int k = 1; k < cnum1; k++) {
                        strcat(listmail.lstr, name1[k]);
                        strcat(listmail.lstr, space);
                        //printf("lstr-%d  %s",k, listmail.lstr);
                    }
                    printf("gonna LIST : %s\n",listmail.lstr);
                    mailbox_send(mailbox_open(smp.from),&listmail);
                } else if(inRoom(id2, cnum2, smp.from)) {
                    char space[3] = "; ";

                    strcpy(listmail.lstr, name2[0]);
                    strcat(listmail.lstr, space);
                    for(int k = 1; k < cnum2; k++) {
                        strcat(listmail.lstr, name2[k]);
                        strcat(listmail.lstr, space);
                    }
                    printf("gonna LIST : %s\n",listmail.lstr);
                    mailbox_send(mailbox_open(smp.from),&listmail);
                }

            } else if(smp.type == WHISPER) {
                mail_t wm = {0,WHISPER, "",""};
                strcpy(wm.lstr, smp.lstr);

                if(inRoom(id1, cnum1, smp.from)) {
                    for(int k = 0; k < cnum1; k++) {
                        if(id1[k] == smp.from) {
                            strcpy(wm.sstr, name1[k]);
                        }
                    }
                    for(int k = 0; k < cnum1; k++) {
                        if(strcmp(name1[k],smp.sstr) == 0) {
                            mailbox_send(mailbox_open(id1[k]),&wm);
                        }
                    }
                } else if(inRoom(id2, cnum2, smp.from)) {
                    for(int k = 0; k < cnum2; k++) {
                        if(id2[k] == smp.from) {
                            strcpy(wm.sstr, name2[k]);
                        }
                    }
                    for(int k = 0; k < cnum1; k++) {
                        if(strcmp(name2[k],smp.sstr) == 0) {
                            mailbox_send(mailbox_open(id2[k]),&wm);
                        }
                    }
                }

            }


        } else {

        }

    }


    //close the chatroom
    printf("server has been shut down.\n");
    mailbox_close(server_t);
    mailbox_unlink(0);
    return 0;
}
