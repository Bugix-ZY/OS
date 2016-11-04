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
#define LIST		5
#define WHISPER	    6
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
        //printf("first time build: ");
        printf("%s.\n",address);
        fd = shm_open(boxName,O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
        mb->fd = fd;
        ftruncate(fd, SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
        //printf("mailbox.size = %d\n",SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
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
    //printf("mailbox.size = %d\n",SIZE_OF_MAIL*BOX_SIZE + SIZE_OF_4_CHAR*2);
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
    //printf("send-head=%d\n",countMail);
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



/**********************************************************************/
/**********************************************************************/
/**********************************************************************/

int main()
{
	mailbox_t server_t;
	mailbox_t client;
	int ID = 1;

	char address[20] = "/dev/shm/__mailbox_";
	char mailBoxId[5];
	sprintf(mailBoxId,"%d",ID);
    strcat(address,mailBoxId);

    while(access(address,F_OK) == 0) {
    	ID += 1;
		strcpy(address,"/dev/shm/__mailbox_");
		sprintf(mailBoxId,"%d",ID);
    	strcat(address,mailBoxId);
    }


 	char ans[5];
   	char usrname[16];
   	printf("\nDo you want to join the chatroom? (yes/no):  ");
   	scanf("%s",ans);
   	if(strcmp(ans, "yes") == 0){
    	printf("please enter your name:  ");
   		scanf("%s",usrname);


   		char c[1];
		printf("which chatroom do you want to join? < 1 or 2 >: ");
		scanf("%s",c);
   		while((strcmp(c,"1") != 0 && strcmp(c, "2") != 0)){
   			printf("please input the right number:  ");
   			scanf("%s",c);
   		}

    	client = mailbox_open(ID);
   		server_t = mailbox_open(0);
   		mail_t smail = {ID,JOIN,"",""};
   		strcpy(smail.lstr,c);
   		strcpy(smail.sstr,usrname);
   		printf("-------------------------------------------\n");
    	mailbox_send(server_t,&smail);
   	} else {
   		printf("goodbye\n" );
   		return 0;
   	}


    struct timeval tv;
    fd_set fds;
    tv.tv_sec = 0;
    tv.tv_usec = 0;
    //server deal with mails
    char buf[SIZE_OF_LONG_STRING];
    memset(buf, 0, sizeof(buf));
    int nread;
    int flag = fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    
    
    while(TRUE) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO,&fds);
        select(STDERR_FILENO+1, &fds,NULL, NULL, &tv);

        if(FD_ISSET(0,&fds)){
            ioctl(0,FIONREAD, &nread);
            nread = read(0, buf, nread);
            buf[nread] = 0;
            printf("\033[1A");
            printf("\033[K");
            //write mail;
            mail_t t;
            t.from = ID;
            char list[5] = "LIST";
            char leave[6] = "LEAVE";
            char whisp[6] = "WHISP";

            if(strncmp(buf,list,4) == 0){
            	t.type = LIST;
            	strcpy(t.lstr, buf);
            }
            else if(strncmp(buf,leave,5) == 0){
            	t.type = LEAVE;
            	mailbox_send(server_t,&t);
            	break;
            }
            else if(strncmp(buf,whisp,5) == 0){
                fcntl(STDIN_FILENO, F_SETFL, flag & ~O_NONBLOCK);
                t.type = WHISPER;
                
                char secret[SIZE_OF_LONG_STRING];
                char to[SIZE_OF_SHORT_STRING];
                printf("WHISPER MODE, please input the NAME and CONTENT: \n");
                scanf("%s",to);
                gets(secret);
                flag = fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
                strcpy(t.sstr, to);
                strcpy(t.lstr, secret);
                //flag = fcntl(STDIN_FILENO, F_SETFL, O_NONBLOCK);
    
            }
            else{
            	t.type = BROADCAST;
            	strcpy(t.lstr, buf);
            	printf("%s: %s",usrname, buf );
        	}
            mailbox_send(server_t,&t);
            //printf("David: %s",buf);
        }

        mail_t m;
        if(mailbox_recv(client,&m) == SUCCESS) {
            if(m.type == BROADCAST)
                printf("%s: %s",m.sstr,m.lstr);
            else if(m.type == JOIN){
                printf("system > %s has joined the chatroom!\n",m.sstr);
            }
            else if(m.type == LIST){
            	printf("online users: %s\n", m.lstr);
            }
            else if(m.type == WHISPER){
                printf("%s whispered to you: %s\n", m.sstr, m.lstr);
            }
        } else{

        }

    }

    printf("You have left the chatroom.\n");
    mailbox_close(client);
    mailbox_unlink(ID);

    return 0;
}
