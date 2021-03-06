#include <sys/types.h>
#include <pwd.h>
#include <shadow.h>
#include <crypt.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <termios.h>
#include <time.h>
#include "stringutil.h"

#ifndef AS_FILE
#define AS_FILE "/etc/allowed_to_run_as_root"
#endif

#ifndef ENV_FILE
#define ENV_FILE "/etc/secure_environment_vars"
#endif


#ifndef SECURE_PATH
#define SECURE_PATH "PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin"
#endif

#ifndef ROOT_USER_NAME
#define ROOT_USER_NAME "root"
#endif

#ifndef NO_FUNNY_STUFF
const char* stallman_dating = 
"I'm a single atheist white man, 55, reputedly intelligent, with unusual interests in politics, science, music and dance. I'd like to meet a woman with varied interests, curious about the world, comfortable expressing her likes and dislikes (I hate struggling to guess), delighting in her ability to fascinate a man and in being loved tenderly, who values joy, truth, beauty and justice more than \"success\"—so we can share bouts of intense, passionately kind awareness of each other, alternating with tolerant warmth while we're absorbed in other aspects of life. My 25-year-old child, the Free Software Movement, occupies most of my life, leaving no room for more children, but I still have room to love a sweetheart if she doesn't need to spend time with me every day. I spend a lot of my time traveling to give speeches, often to Europe, Asia and Latin America; it would be nice if you were free to travel with me some of the time.If you are interested, write to rms at gnu dot org and we'll see where it leads."
;
#endif

static char I_hate_buffers[0x1000000];
static char I_hate_buffers2[0x1000000];
static char* old_term = NULL;
static const unsigned long bcap = 0x1000000;
struct termios t_old;
struct termios t_new;
int have_saved = 0;

void toggleEcho(int EchoOn)
{
#ifndef MY_TERMINAL_SUCKS
	if(EchoOn == 0 && !have_saved){
		tcgetattr(STDIN_FILENO, &t_old);
		/*t_new.c_lflag &= ~(ICANON | ECHO);*/
		t_new = t_old;
		t_new.c_lflag &= ~(ICANON | ECHO);
		tcsetattr(STDIN_FILENO, TCSANOW, &t_new);
		have_saved = 1;
	} else if (have_saved){
		have_saved = 0;
		tcsetattr(STDIN_FILENO, TCSANOW, &t_old);
	}
#endif
}

static void fail_not_on_list(){
	printf("\r\nYou ain't on the list. You can't run things as root. People with four question marks around their name in " AS_FILE 
	"can run thing as root without a password, like ??fred??, or exclamation marks like !!fred!!\r\n");
	exit(1);
}

static void fail_funny_name(){
	printf("\r\nFunny guy, huh? Sorry, that's not how we do things around here.\r\n");
	exit(1);
}

/* 0 means success.*/
static int CheckPassword( const char* user, const char* password )
{
    struct passwd* passwdEntry = getpwnam( user );
    if ( !passwdEntry )
    {
        printf( "User '%s' doesn't exist\n", user );
        return 1;
    }

    if ( 0 != strcmp( passwdEntry->pw_passwd, "x" ) )
    {
        return strcmp( passwdEntry->pw_passwd, crypt( password, passwdEntry->pw_passwd ) );
    }
    else
    {
		/* password is in shadow file*/
        struct spwd* shadowEntry = getspnam( user );
        if ( !shadowEntry )
        {
            return 1;
        }
        return strcmp( shadowEntry->sp_pwdp, crypt( password, shadowEntry->sp_pwdp ) );
    }
}



static char* read_until_terminator_alloced_modified(){
	unsigned char c;
	unsigned long blen = 0;
	unsigned long dating_progress = 0;
	while(1){
		if(feof(stdin)){
			fail_funny_name();
		}
		c = getchar();
		
		if(c == '\n' || c=='\r') {break;}
		if(c > 127) continue; /*Pretend it did not happen.*/
		if(c < 8) continue; /*Also didn't happen*/
		if(
			c == 127
			|| c==8
		)
		{
			{if(blen) printf("[Retype that.]\r\n");}
			blen = 0;
			continue;
		}
		#ifndef NO_FUNNY_STUFF
		#ifndef MY_TERMINAL_SUCKS
		/*wow this is funny*/
		/*
		putchar(0x1B);putchar('[');putchar('D');
		putchar(' ');
		putchar(0x1B);putchar('[');putchar('D');
		*/
		{
			unsigned long chars_to_add = rand() & 15;
			for(;chars_to_add;chars_to_add--,dating_progress++)
			{
				unsigned long charindex;
				charindex = dating_progress % strlen(stallman_dating);
				putchar(stallman_dating[charindex]);
			}
		}
		#endif
		#endif
		if(blen == (bcap-1))	/*Don't even.*/
			{
				fail_funny_name();
			}
		I_hate_buffers[blen++] = c;
	}
	I_hate_buffers[blen] = '\0';
	printf("\r\n");
	return I_hate_buffers;
}

int main(int argc, char** argv){
	char* pwd_entry = NULL;
	struct passwd p;
	struct passwd *p_ptr = NULL;
	char* name_formatted = NULL;
	char* text = NULL;
	
#ifndef KEEP_MY_ENVIRONMENT



#ifndef NO_DONT_GET_THE_OLD_TERM
	if(getenv("TERM") && strlen(getenv("TERM"))){
		old_term = strcatalloc("TERM=", getenv("TERM"));
		if(!old_term) {printf("\r\nMalloc Failed?!?!\r\n");return 1;}
	}
#endif


	clearenv();
	putenv(SECURE_PATH);


#ifndef NO_DONT_GET_THE_OLD_TERM
	if(old_term) putenv(old_term);
#endif


	{char* text2 = NULL;
		{
			unsigned long len;
			FILE* f = fopen(ENV_FILE, "r");
			if(!f){
				printf("\r\nCannot open ENV file.\r\n");
				return 1;
			}
			text2 = read_file_into_alloced_buffer(f, &len);
			fclose(f);
		}
		if(!text2) {printf("\r\nMalloc Failed?!?!\r\n");return 1;}
		/*Parse that file!*/
		{char *scroller = text2;
			long loc = 0;
			for(;;)
			{
				loc = strfind(scroller, "\n");
				if(loc == -1) break;
				scroller[loc] = '\0';
				if(!strprefix("#",scroller) && (strlen(scroller) > 2)){
					putenv(scroller);
				}
				scroller += loc + 1;
			}
		}
		/*free(text2);*/
	}
#else
#ifndef KEEP_MY_PATH_TOO
putenv(SECURE_PATH);
#endif
#endif
	if(getpwuid_r(getuid(), &p, I_hate_buffers2,
	           0x1000000, &p_ptr))
	{
		printf("\r\nI can't identify you.\r\n");
		return 1;
    };
	srand(time(NULL));
	if(argc < 1) return 1; /*Don't even*/
	if(argc < 2) {
		printf("\r\nadmin, a doas/sudo replacement program for linux\r\n");
		printf("wow!!! systemd is bad. install djentoo. sudo bloat. vim master race!!!!\r\n");
		printf("usage: you just run programs like normal, but you type %s in front.\r\n", argv[0]);
		printf("there's a file called " AS_FILE " which contains a list of whose allowed to be root,\r\n");
		printf("people with two question marks at either side (total: 4) of their name get to be root without typing their password.\r\n");
		printf("Those with exclamation marks (total: 4) must type in their password.\r\n");
		printf("An additional file is required called " ENV_FILE " which contains the secure path to use. Environment sanitization is done before running programs, you see.\r\n");
		return 1;
	}
	if(p_ptr == NULL || p.pw_name == NULL) {
		printf("\r\nI can't identify you.\r\n");
		return 1;
	}
	
	if(strlen(p.pw_name) > 3000) fail_funny_name();  /*prohibit that sit.*/
	if(strlen(p.pw_name) < 1) fail_funny_name();  /*prohibit that sit.*/
	if(strfind(p.pw_name, "!!") != -1) fail_funny_name();
	if(strfind(p.pw_name, "??") != -1) fail_funny_name();
	name_formatted = strcatalloc(p.pw_name,"??");
	if(!name_formatted) {printf("\r\nMalloc Failed?!?!\r\n");return 1;}
	name_formatted = strcatallocf2("??", name_formatted);
	if(!name_formatted) {printf("\r\nMalloc Failed?!?!\r\n");return 1;}
	{
		long lucky;
		{
			unsigned long len;
			FILE* f = fopen(AS_FILE, "r");
			if(!f){
				printf("\r\nCannot open AS_FILE\r\n");
				return 1;
			}
			text = read_file_into_alloced_buffer(f, &len);
			fclose(f);
		}
		if(!text) {printf("\r\nMalloc Failed?!?!\r\n");return 1;}
		
		lucky = strfind(text, name_formatted);
		if(lucky == -1){
			goto check_passworded;
		}
		/*passwordless*/
			{
				struct passwd* rp = getpwnam(ROOT_USER_NAME);
				if(!rp) {printf("\r\nInternal Error.\r\n"); exit(1);}
				setreuid(rp->pw_uid,rp->pw_uid);
				setregid(rp->pw_gid,rp->pw_gid);
				execvp(argv[1],argv+1);
			}
		return 0;
	}


	check_passworded:
	/*printf("\r\nHello '%s'!\r\n", p.pw_name);*/
	free(name_formatted); name_formatted = NULL;
	name_formatted = strcatalloc(p.pw_name,"!!");
	if(!name_formatted) {printf("\r\nMalloc Failed?!?!\r\n");return 1;}
	name_formatted = strcatallocf2("!!", name_formatted);
	if(!name_formatted) {printf("\r\nMalloc Failed?!?!\r\n");return 1;}
	{
		long lucky;
		lucky = strfind(text, name_formatted);
		if(lucky == -1){
			fail_not_on_list();
		}
		/*passworded.*/
		printf("\r\nPassword?\r\n");
		toggleEcho(0);
			pwd_entry = read_until_terminator_alloced_modified();
		toggleEcho(1);
		if(
			CheckPassword(p.pw_name, pwd_entry) == 0
		){
			{
				struct passwd* rp = getpwnam(ROOT_USER_NAME);
				if(!rp) {printf("\r\nInternal Error.\r\n"); exit(1);}
				setreuid(rp->pw_uid,rp->pw_uid);
				setregid(rp->pw_gid,rp->pw_gid);
				execvp(argv[1],argv+1);
			}
		} else {
			printf("\r\n<Wrong Password, idiot!>\r\n");
			return 1;
		}
	}
	return 0;
}
