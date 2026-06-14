#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <sys/stat.h>

enum COMMAND { NONE=0, RUN=10, EXEC=11 };

struct config {
    enum COMMAND subcommand;
    char name[64];
    char command[256];
};

void write_file(const char *path, const char *value) {
    int fd = open(path, O_WRONLY);
    if (fd < 0) { perror(path); exit(1); }
    if (write(fd, value, strlen(value)) < 0) { perror(path); close(fd); exit(1); }
    close(fd);
}

int validate_config(struct config cfg) {
    if (cfg.subcommand == NONE) { fprintf(stderr, "[ERR] Missing subcommand (run|exec)\n"); return 1; }
    if (strcmp(cfg.name,"")==0) strncpy((char*)cfg.name,"bib",sizeof(cfg.name)-1);
    if (strcmp(cfg.command,"")==0) { fprintf(stderr,"[ERR] Missing command\n"); return 1; }
    return 0;
}

int run_container(struct config cfg) {
    pid_t pid;
    uid_t host_uid=getuid();
    gid_t host_gid=getgid();
    if (unshare(CLONE_NEWUSER | CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWUTS | CLONE_NEWTIME) != 0) { perror("[ERR] unshare"); return 1; }
    if (access("/proc/self/setgroups",F_OK)==0) write_file("/proc/self/setgroups","deny");
    char buf[128];
    snprintf(buf,sizeof(buf),"0 %u 1\n",(unsigned)host_uid); write_file("/proc/self/uid_map",buf);
    snprintf(buf,sizeof(buf),"0 %u 1\n",(unsigned)host_gid); write_file("/proc/self/gid_map",buf);
    if (mount(NULL,"/",NULL,MS_REC|MS_PRIVATE,NULL)!=0){ perror("[ERR] mount /"); return 1; }
    pid=fork();
    if (pid<0){ perror("[ERR] fork"); return 1; }
    if (pid==0){
        if (sethostname(cfg.name,strlen(cfg.name))!=0){ perror("[ERR] sethostname"); exit(1); }
        if (mount("proc","/proc","proc",0,NULL)!=0){ perror("[ERR] mount /proc"); exit(1); }
        execl("/bin/sh","sh","-c",cfg.command,NULL);
        perror("[ERR] execl"); exit(1);
    }else{
        waitpid(pid,NULL,0);
        printf("[Parent] Container stopped.\n");
    }
    return 0;
}

int main(int argc,char **argv){
    struct config cfg;     memset(&cfg, 0, sizeof(cfg));     cfg.subcommand = NONE;
    int i=1;
    while(i<argc){
        if(strcmp(argv[i],"run")==0){ cfg.subcommand=RUN; i++; }
        else if(strcmp(argv[i],"exec")==0){ cfg.subcommand=EXEC; i++; }
        else if(strcmp(argv[i],"--name")==0){
            if(i+1>=argc){ fprintf(stderr,"[ERR] Missing --name value.\n"); return 1; }
            strncpy(cfg.name,argv[++i],sizeof(cfg.name)-1); i++;
        }else{ strncpy(cfg.command,argv[i],sizeof(cfg.command)-1); i++; }
    }
    if(validate_config(cfg)!=0) return 1;
    switch(cfg.subcommand){
        case RUN: if(run_container(cfg)!=0){ fprintf(stderr,"[ERR] Running container failed.\n"); return 1; } break;
        case EXEC: printf("[INFO] EXEC subcommand not implemented yet.\n"); break;
        default: break;
    }
    return 0;
}
