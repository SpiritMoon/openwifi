#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <string.h>
#include <linux/sockios.h>  


typedef struct cpu_info         //����һ��cpu occupy�Ľṹ��
{
unsigned int user; //����һ���޷��ŵ�int���͵�user
unsigned int nice; //����һ���޷��ŵ�int���͵�nice
unsigned int system;//����һ���޷��ŵ�int���͵�system
unsigned int idle; //����һ���޷��ŵ�int���͵�idle
unsigned int iowait;
unsigned int irp;
unsigned int softirp;
unsigned int stealstolen;
unsigned int guest;
}cpu_occupt;


typedef struct ap_local_info         //����һ��mem occupy�Ľṹ��
{
unsigned long mem_total; 
unsigned long mem_free;
double cpu_idle_rate;
}ap_local_info_t;


get_memoccupy (ap_local_info_t *mem) //��������get��������һ���βνṹ����Ū��ָ��O
{
    FILE *fd;          
    int n;             
    char buff[256];   
    ap_local_info_t *m;
	char tmp[30];
    m=mem;
	
    system("cat /proc/meminfo | sed -n '1,2p' > /workPorjectCode/t/cpuinfo.txt"); 
	
    fd = fopen ("/workPorjectCode/t/cpuinfo.txt", "r"); 
 
    fgets (buff, sizeof(buff), fd); 
    sscanf (buff, "%s %u", tmp, &m->mem_total); 
    fgets (buff, sizeof(buff), fd); 
    sscanf (buff, "%s %u", tmp, &m->mem_free); 
    printf ("%u %u\n", tmp, m->mem_total, m->mem_free); 

    fclose(fd);     //�ر��ļ�fd
}

double cal_cpuoccupy (cpu_occupt *o, cpu_occupt *n) 
{   
    unsigned long one, two;    
    unsigned long idle, sd;
	unsigned long totalcputime;
    double cpu_use = 0;   

    one = (unsigned long) (o->user + o->nice + o->system +o->idle+o->iowait+o->irp+ o->softirp+ o->stealstolen+ o->guest);//��һ��(�û�+���ȼ�+ϵͳ+����)��ʱ���ٸ���od
    two = (unsigned long) (n->user + n->nice + n->system +n->idle+n->iowait+n->irp+ n->softirp+ n->stealstolen+ n->guest);//�ڶ���(�û�+���ȼ�+ϵͳ+����)��ʱ���ٸ���od
      
    idle = (unsigned long) (n->idle - o->idle);    //�û���һ�κ͵ڶ��ε�ʱ��֮���ٸ���id
    //sd = (unsigned long) (n->system - o->system);//ϵͳ��һ�κ͵ڶ��ε�ʱ��֮���ٸ���sd
	
	totalcputime = two-one;
    if(totalcputime != 0)
    cpu_use = (idle)*100.0/totalcputime; //((�û�+ϵͳ)��100)��(��һ�κ͵ڶ��ε�ʱ���)�ٸ���g_cpu_used
    else cpu_use = 0.0;
    return cpu_use;
}

get_cpuoccupy (cpu_occupt *cpust) //��������get��������һ���βνṹ����Ū��ָ��O
{   
    FILE *fd;         
    int n;            
    char buff[256]; 
	char tmp[30];
    cpu_occupt *cpu_occupt;
    cpu_occupt=cpust;
             
    system("cat /proc/stat | sed -n '1,1p' > /workPorjectCode/t/cpuinfo.txt;cat /proc/stat | sed -n '1,1p' >> /workPorjectCode/t/cpuinfo.txt"); 
	
    fd = fopen ("/workPorjectCode/t/cpuinfo.txt", "r"); 
    fgets (buff, sizeof(buff), fd);
    
    sscanf (buff, "%s %u %u %u %u %u %u %u %u %u",
		tmp, &cpu_occupt->user, &cpu_occupt->nice,&cpu_occupt->system, &cpu_occupt->idle, &cpu_occupt->iowait,
		&cpu_occupt->irp, &cpu_occupt->softirp, &cpu_occupt->stealstolen, &cpu_occupt->guest );

    printf ("%s %u %u %u %u %u %u %u %u %u\n",
		tmp, cpu_occupt->user, cpu_occupt->nice,cpu_occupt->system, cpu_occupt->idle, cpu_occupt->iowait,
		cpu_occupt->irp, cpu_occupt->softirp, cpu_occupt->stealstolen, cpu_occupt->guest );

    fclose(fd);     
}

int main()
{
    cpu_occupt cpu_stat1;
    cpu_occupt cpu_stat2;
    ap_local_info_t ap_info;
    double cpu_idle;
    
		//��ȡ�ڴ�
		get_memoccupy ((ap_local_info_t *)&ap_info);
    
	
		//��һ�λ�ȡcpuʹ�����
		get_cpuoccupy((cpu_occupt *)&cpu_stat1);
		sleep(1);
		//�ڶ��λ�ȡcpuʹ�����
		get_cpuoccupy((cpu_occupt *)&cpu_stat2);
		
		//����cpuʹ����
		cpu_idle = cal_cpuoccupy ((cpu_occupt *)&cpu_stat1, (cpu_occupt *)&cpu_stat2);
		printf("%f\n",cpu_idle);

    return 0;
}