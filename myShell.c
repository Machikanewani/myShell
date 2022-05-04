#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#define BUFLEN 256
#define CMDNUM 9

int parse(char *, char **);
void cd(char **, int);
void pushd(char **, int, char **);
void dirs(char **);
void popd(char **);
void history(char **);
int find(char **);

char *command[CMDNUM] = {"cd","pushd","dirs","popd","history","prompt","alias","unalias","find"};

int i_dirstack = 20 - 1;
int i_history = 0;
int i_aliashis = 0;

int main(int argc, char * argv[]){
  char cmd_buffer[BUFLEN];
  char **args = (char **)malloc(20 * sizeof(char *));
  char *prompt = (char *)malloc(20 * sizeof(char));
  char **dirstack = (char **)malloc(20 * sizeof(char *));
  char **cmdhis = (char **)malloc(32 * sizeof(char *));
  char **pathdir = (char **)malloc(32 * sizeof(char *));
  char **aliashis = (char **)malloc(CMDNUM * sizeof(char *));
  char tmp[BUFLEN];

  char *cmd_cd = "cd";
  char *cmd_pushd = "pushd";
  char *cmd_dirs = "dirs";
  char *cmd_popd = "popd";
  char *cmd_history = "history";
  char *cmd_prompt = "prompt";
  char *cmd_alias = "alias";
  char *cmd_unalias = "unalias";
  char *cmd_find = "find";
  
  char *functionarray[CMDNUM] = {cmd_cd,cmd_pushd,cmd_dirs,cmd_popd,cmd_history,cmd_prompt,cmd_alias,cmd_unalias,cmd_find};

  prompt = "Command";
  int k;
  int cmd_status;

  // PATH directory
  char *pathtmp = (char *)malloc((strlen(getenv("PATH")) + 1) * sizeof(char));
  pathtmp = getenv("PATH");

  char *copypath = (char *)malloc((strlen(pathtmp) + 1) * sizeof(char));
  strcpy(copypath,pathtmp);

  char *p;
  int i;
  p = copypath;
  for(i = 0; i < 32; i++){
    if((pathdir[i] = strtok(p, ":")) == NULL){
      break;
    }
    p = NULL;
  }

new:

  for(;;){
    int cnt_args = 0;

    printf("%s:",prompt);
  
    if(fgets(cmd_buffer,BUFLEN,stdin) == NULL){
      if(feof(stdin)){
        printf("exit!\n");
        exit(EXIT_SUCCESS);
      }
      printf("\n");
      continue;
    }
    
    *(cmd_buffer + strlen(cmd_buffer) - 1) = '\0';
    char *cmdcpy = (char *)malloc((strlen(cmd_buffer) + 1) * sizeof(char));
    strcpy(cmdcpy,cmd_buffer);
   

exe:
    if(strstr(cmd_buffer,"!") == NULL){ // history
      if(i_history < 32){
        cmdhis[i_history] = (char *)malloc((strlen(cmd_buffer)+1) * sizeof(char));
        strcpy(cmdhis[i_history],cmd_buffer);
        i_history++;
      }
      else{
        for(k = 0; k < 31; k++){
          free(cmdhis[k]);
          cmdhis[k] = (char *)malloc((strlen(cmdhis[k + 1]) + 1) * sizeof(char));
          strcpy(cmdhis[k],cmdhis[k + 1]);
        }
        free(cmdhis[k]);
        cmdhis[k] = (char *)malloc((strlen(cmd_buffer) + 1) * sizeof(char));
        strcpy(cmdhis[k],cmd_buffer);
      }
    }
    
    // 切り分け
    cmd_status = parse(cmd_buffer, args);
    
    // exit if "exit"
    if(cmd_status == 2){
      printf("done.\n");
      free(pathdir);
      free(dirstack);
      free(cmdhis);
      free(args);
      free(aliashis);
      exit(EXIT_SUCCESS);
    }
    else if(cmd_status == 3) continue;
    while(args[cnt_args] != NULL) cnt_args++;
    
    // external command check
    int incmdflag = 0;
    char maindir[256];
    getcwd(maindir, sizeof(maindir));

    for(i = 0; i < CMDNUM; i++){
      if(strcmp(args[0], functionarray[i]) == 0){
        incmdflag = 1;
        break;
      }
    }

    if(incmdflag != 1){ // not created func
      DIR *excmddp;
      struct dirent *excmddir;

      int excmdflag = 0;
      for(i = 0; i < 32 && pathdir[i] != NULL; i++){
        chdir(pathdir[i]);

        excmddp = opendir(".");
        
        while((excmddir = readdir(excmddp)) != NULL){
          if(strcmp(args[0],excmddir->d_name) == 0){
            excmdflag = 1;
            break;
          }

        }
        if(excmdflag) break;
      }
      
      chdir(maindir);
      if(excmdflag){
        int status;
        pid_t pid;

        if((pid = fork()) < 0){
          perror("fork");
          exit(1);
        }
        else if(pid == 0){
          args[cnt_args] = NULL;
          execvp(*args,args);
          perror(args[0]);
          exit(1);
        }
        else{
          while (wait(&status) != pid);
        }
        printf("\n");
        goto new;
      }
    } // external ends
  
    char *fstarg;
    fstarg = args[0];

    // *
    for(int i = 0; args[i] != NULL; i++){
      if(strcmp(args[i],"*") == 0){

        int lastflag = 0;
        if(i == cnt_args - 1) lastflag = 1;

        char *tmparg = (char *)malloc(sizeof(char));
        strcpy(tmparg,args[cnt_args - 1]);
        
        DIR *dp;
        struct dirent *directory;
        struct stat filestat;

        dp = opendir(".");

        while((directory = readdir(dp)) != NULL){
          if(strcmp(directory->d_name,".") == 0 || strcmp(directory->d_name,"..") == 0)
          continue;
          
          if(stat(directory->d_name,&filestat) != 0){
            perror("main");
            exit(1);
          }
          else{
            if(S_ISREG(filestat.st_mode)){
              args[i] = (char *)malloc((strlen(directory->d_name) + 1) * sizeof(char));
              strcpy(args[i],directory->d_name);
              i++;
            }
          }
        }
        if(lastflag == 0){
          args[i] = (char *)malloc((strlen(tmparg) + 1) * sizeof(char));
          strcpy(args[i],tmparg);
          i++;
        }
        i--;
        
        int j;
        for(j = 0; j < i + 1; j++){
          printf(" args[%d] = %s\n",j, args[j]);
        }
        cnt_args = j - 1;
        free(tmparg);
        break;
      }
    }
    

    // !!
    if(strcmp(fstarg,"!!") == 0){
      i_history--;
      if(i_history < 0){
        printf("No command stored!\n");
        continue;
      }
      strcpy(cmd_buffer,cmdhis[i_history]);
      i_history++;
      goto exe;
    }
   

    // !string
    if(*fstarg == '!'){
      int i = i_history - 1;
      int j = 1;

      if(strlen(fstarg) == 1){
        printf("Want string\n\n");
        continue;
      }
      
      // !n
      if(isdigit(*(fstarg + 1)) != 0){
        int tens = 0;
        int ones = 0;
        int num = 0;
        // get number
        if(strlen(fstarg) == 2){ // one digit
          num = *(fstarg + 1) - '0';
        }
        else if(strlen(fstarg) == 3){ // two digits
          tens = *(fstarg + 1) - '0';
          ones = *(fstarg + 2) - '0';
          num = 10 * tens + ones;
        }
        else{
          printf("Too big number!\n\n");
          continue;
        }
         
        if(num > i_history || num == 0){
          printf("Too big number!\n\n");
          continue;
        }
        else{
          strcpy(cmd_buffer,cmdhis[num - 1]);
          goto exe;
        }
      }
      // !-n
      else if(*(fstarg + 1) == '-'){
        int tens = 0;
        int ones = 0;
        int num = 0;
        
        // get number
        if(strlen(fstarg) == 3){ // one digit
          num = *(fstarg + 2) - '0';
        }
        else if(strlen(fstarg) == 4){ // two digits
          tens = *(fstarg + 2) - '0';
          ones = *(fstarg + 3) - '0';
          num = 10 * tens + ones;
        }
        else{
          printf("Too big number!\n\n");
          continue;
        }

        if(num > i_history || num == 0){
          printf("Too big number!\n\n");
          continue;
        }
        else{
          strcpy(cmd_buffer,cmdhis[i_history - num]);
          goto exe;
        }
      }
      else{
        for(; *(fstarg + j) != '\0'; j++){
          tmp[j - 1] = *(fstarg + j);
        }
        tmp[j - 1] = '\0';
      
        for(;cmdhis[i] != NULL; i--){
          if(strstr(cmdhis[i],tmp) != NULL){
            strcpy(cmd_buffer,cmdhis[i]);
            goto exe;
          }
        }
      }
    }
    

    // function
    if(strcmp(fstarg,functionarray[0]) == 0){ //cd
      cd(args,cnt_args);
    }
    else if(strcmp(fstarg,functionarray[1]) == 0){ //pushd
      pushd(args,cnt_args,dirstack);
    }
    else if(strcmp(fstarg,functionarray[2]) == 0){ //dirs
      dirs(dirstack);
    }
    else if(strcmp(fstarg,functionarray[3]) == 0){ //popd
      if(i_dirstack == 19){
        printf("No directory pushed!\n\n");
        continue;
      }
      popd(dirstack);
    }
    else if(strcmp(fstarg,functionarray[4]) == 0){ //history
      history(cmdhis);
    }
    else if(strcmp(fstarg,functionarray[5]) == 0){ //prompt
      if(cnt_args > 2) printf("Invalid prompt!\n");
      else if(cnt_args == 1){
        free(prompt);
        prompt = "Command";
        printf("\n");
        continue;
      }
      else{
        prompt = (char *)malloc((strlen(args[1]) + 1) * sizeof(char));
        strcpy(prompt,args[1]);
        printf("\n");
        continue;
      }
    }
    else if(strcmp(fstarg,functionarray[6]) == 0){ //alias
      if(cnt_args > 3 || cnt_args == 2){
        printf("Invalid alias!\n\n");
        continue;
      }
      else if(cnt_args == 1){
          if(i_aliashis == 0){
            printf("No change have been made!\n\n");
            continue;
          }
          for(i = 0; i < i_aliashis; i++){
              printf(" %s\n",aliashis[i]);
          }
      }
      else{
        for(i = 0; i < CMDNUM; i++){
          if(strcmp(functionarray[i],args[2]) == 0){
            functionarray[i] = (char *)malloc((strlen(args[1]) + 1) * sizeof(char));
            strcpy(functionarray[i],args[1]);
            
            aliashis[i_aliashis] = (char *)malloc(30 * sizeof(char));
            
            strcpy(aliashis[i_aliashis], strstr(cmdcpy,args[1]));
            i_aliashis++;
            break;
          }
        }
        printf("\n");
        continue;
      }
    }
    else if(strcmp(fstarg,functionarray[7]) == 0){ //unalias
      if(cnt_args != 2){
        printf("Invalid unalias!\n\n");
        continue;
      }
      int changeflag = 0;
      for(int i = 0; i < CMDNUM; i++){
        if(strcmp(functionarray[i],args[1]) == 0){
          free(functionarray[i]);
          functionarray[i] = (char *)malloc((strlen(command[i]) + 1) * sizeof(char));
          strcpy(functionarray[i],command[i]);

          for(i = 0; i < i_aliashis; i++){
            if(strstr(aliashis[i],args[1]) != NULL){
              if(i == i_aliashis - 1){
                aliashis[i] = NULL;
                i_aliashis--;
                changeflag = 1;
                break;
              }
              else{
                for(int j = i; j < i_aliashis - 1; j++){
                  free(aliashis[i]);
                aliashis[i] = (char *)malloc((strlen(aliashis[i + 1] + 1) * sizeof(char)));
                strcpy(aliashis[i],aliashis[i + 1]);
                aliashis[i + 1] = NULL;
                }
                i_aliashis--;
                changeflag = 1;
                break;
              }
            }
          }
        }
      }
      if(changeflag == 0) printf("Wrong name!\n");
      printf("\n");
      continue;
    }
    else if(strcmp(fstarg,functionarray[8]) == 0){ //find
      if(find(args) == 0) printf("No such file or directory!\n");
      chdir(maindir);
    }
    else printf("Wrong command!\n");
    printf("\n");
  }
}

int parse(char buffer[], char * args[]){
  int i_args = 0;
  int status = 0;
  char *p;
  
  if(strcmp(buffer,"exit") == 0){
    status = 2;
    return status;
  }
  p =buffer;
  for(;i_args < 10 ; i_args ++){
    if((args[i_args] = strtok(p," ")) == NULL){
      break;
    }
    p = NULL;
  }
  args[i_args] = NULL;
  if(i_args == 0) status = 3;

  return status;
}

void cd(char *c_args[],int i_a){
  char wd[128];
  char *home;

  if((getcwd(wd,sizeof(wd))) == NULL){
    perror("getcwd failed\n");
    exit(1);
  }
  home = getenv("HOME");
  printf("Current directory:%s\n",getcwd(wd,sizeof(wd)));

  if(i_a == 1) chdir(home);
  else if(i_a > 2){
    printf("Too many directories!\n");
    return;
  }
  else{
    if(chdir(c_args[1]) == -1){
      printf("No such directory!\n");
      return;
    }
  }
  printf("Change to: %s\n",getcwd(wd,sizeof(wd)));
}

void pushd(char *p_args[], int p_a, char **dirstack){
  char wd[128];
  getcwd(wd, sizeof(wd));
  dirstack[i_dirstack] = (char *)malloc((strlen(wd) + 1) * sizeof(char));
  strcpy(dirstack[i_dirstack], wd);
  printf("[%s] is pushed!\n",wd);
  i_dirstack--;
  return;
}

void dirs(char **dirstack){
  if(i_dirstack == 19){
    printf("No directory stacked\n");
    return;
  }
  printf("Directory Stack: \n");
  
  for(int i_dirs = i_dirstack+1;i_dirs < 20; i_dirs++) {
    printf("%s\n",dirstack[i_dirs]);
  }
  return;
}

void popd(char **dirstack){
  char wd[128];

  i_dirstack++;
  printf("(popd)Current dir: %s\n",getcwd(wd,sizeof(wd)));
  if(chdir(dirstack[i_dirstack]) == -1){
    printf("popd failed\n");
    return;
  }
  dirstack[i_dirstack] = NULL;
  printf("(popd)Change to: %s\n",getcwd(wd,sizeof(wd)));
}

void history(char **h_history){
  printf("command history:\n");
  for(int i = 0; i < i_history; i++){
    printf("  [%d] %s\n",i + 1,h_history[i]);
  }
}


int find(char **args){
  DIR *dp;
  char wd[128];
  struct stat filestat;
  struct dirent *directory;
  static int found = 0;

  dp = opendir(".");

  while((directory = readdir(dp)) != NULL){
    char *curdir;
    getcwd(wd, sizeof(wd));
    
    if(strcmp(directory->d_name,".") == 0|| strcmp(directory->d_name,"..") == 0) continue;

    if(stat(directory->d_name,&filestat) == -1){
      perror("main");
      exit(1);
    }
    else{
      if(S_ISDIR(filestat.st_mode)){
        if(strcmp(directory->d_name,args[1]) == 0){
          printf("[directory]%s: %s\n",directory->d_name,wd);
          found = 1;
        }
        chdir(directory->d_name);
        curdir = (char *)malloc((strlen(wd) + 1) * sizeof(char));
        strcpy(curdir,wd);
        found = find(args);
        chdir(curdir);
        continue;
      }
      else if(S_ISREG(filestat.st_mode)){
        if(strcmp(directory->d_name,args[1]) == 0){
          printf("[file]%s: %s\n",directory->d_name,wd);
          found = 1;
        }
        continue;
      }
      else{
        if(strcmp(directory->d_name,args[1]) == 0){
          printf("[other]%s: %s\n",directory->d_name,wd);
          found = 1;
        }
        continue;
      }
    }
  }
  closedir(dp);
  return found;
}
