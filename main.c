#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#include <string.h>
#include <f2fs_fs.h>

/*
 * numero total de entradas parcial é igual
 *      ao numero de blocos*0,05
 * numero total de entradas final
 *     (16 + (total de entradas parcial * 16)) / 512
 *     if = 0
 *      total de entradas parcial +( (512 - (16 + (total de entradas parcial * 16)))%16)
 *     else
 *      total de entradas parcial + ((512 - (16 + (total de entradas parcial * 16)) % 512))%16)
 *
 */


typedef struct BootSAD{
        unsigned short int sectorSize;
        unsigned int totalEntries; // dado em setores, cada setor 512 bytes
        unsigned char entrySize;
        unsigned char errorForm;
        unsigned long int size;
}__attribute__((packed)) BootSAD;


typedef struct Tabent{
    unsigned char status; // 4bits altos arquivo/diretório, 4 bits baixos (uso ou vazio)
    unsigned long int totalSize; // tamanho total da pasta ou do arquivo
    unsigned int sector; //Setor do cabeçalho do arquivo
    unsigned char dia; // Data da criação/escrita do arquvo/diretório
    unsigned char mes;
    unsigned char ano;
}__attribute__((packed)) Tabent;


typedef struct Datanode{
    unsigned char staus; //setor vazio/cheio
    char data[507]; // Caso pirmeiro nodo da cadeia de byte é nome do arquivo, se não dado do arqivo
    unsigned int sector; // proximo setor a ser lido, caso 0 ultimo setor
}__attribute__ ((__packed__)) Datanode;


typedef struct DataDirnode{
    unsigned char staus; //setor vazio/cheio;
    unsigned int indexTable[126];
    unsigned int sector;
    unsigned char alin[3];
}__attribute__ ((__packed__)) DataDirnode;

typedef struct SAD16{
    FILE *disk;
    BootSAD boot;
    Tabent *table;
}SAD16;



void setBoot(FILE *disk, unsigned numbSectors){
    unsigned int totalParcial= (numbSectors*0.05);
    unsigned int resto= ((16 + (totalParcial * 16)) % 512);
    unsigned int totalentradas=0;
    if(resto!=0){
        totalentradas = totalParcial + ((512-resto)/16);
    }
    fseek(disk,0,SEEK_SET);
    BootSAD boot;
    boot.sectorSize=512;
    boot.totalEntries=totalentradas;
    boot.entrySize=16;
    boot.errorForm=1;
    boot.size=numbSectors*512;
    fwrite(&boot,sizeof(BootSAD),1,disk);
//    printf("boot seted");
}
void setRootEntry(FILE *disk){
    fseek(disk,16,SEEK_SET);
//    time_t *rawtime;
//    struct tm * timeinfo;
//    time (rawtime);
//    timeinfo = localtime (&rawtime);
    time_t timer;
    time(&timer);
    struct tm *timeinfo;
    timeinfo = localtime (&timer);
    Tabent entradaRoot;
    entradaRoot.status=240;
    entradaRoot.totalSize=1;
    entradaRoot.sector=0; //primeiro setor da área de dados é o root
//    entradaRoot.hora=timeinfo->tm_hour;
//    entradaRoot.minuto=timeinfo->tm_min;
    entradaRoot.dia=timeinfo->tm_mday;
    entradaRoot.mes=timeinfo->tm_mon + 1;
    entradaRoot.ano=timeinfo->tm_year;
    fwrite(&entradaRoot, sizeof(Tabent),1,disk);
//    printf("Root Entry seted");
}

void voidDirNode(struct DataDirnode *dataDirnode){
    for(int i=0;i<63;i++){
        dataDirnode->indexTable[i]=0;
    }
}

void setRootNode(FILE *disk,BootSAD boot){
    fseek(disk, 16+(boot.totalEntries*16), SEEK_SET);

    struct DataDirnode datadirnode;
    voidDirNode(&datadirnode);
    datadirnode.sector=0;
    datadirnode.staus=240;
    fwrite(&datadirnode,sizeof(struct DataDirnode),1,disk);
}

void printDirIndex(DataDirnode dirnode){
    for(int i=0; i<63;i++){
        printf("%u ",dirnode.indexTable[i]);
    }
    printf("\n");
}

void printBoot(BootSAD boot){
    printf("%hu \n", boot.sectorSize);
    printf("%u \n", boot.totalEntries);
    printf("%d \n", boot.entrySize);
    printf("%d \n", boot.errorForm);
    printf("%lu \n", boot.size);
}

void format(FILE *disk){
    unsigned int numberOfSector=0;
    scanf("%u",&numberOfSector);

    char blank[512];
    for(int i=0;i<512;i++){
        blank[i]=0;
    }
    for(int i=0;i<numberOfSector;i++)
        fwrite(blank, sizeof(char), sizeof(blank),disk);

    setBoot(disk,numberOfSector);
    fseek(disk,0,SEEK_SET);
    BootSAD boot;
    printf("\nBOOT\n");
    fread(&boot, sizeof(BootSAD),1,disk);

    printBoot(boot);

    setRootEntry(disk);
//    Tabent entradaRoot1;
//    fseek(disk,16,SEEK_SET);
//    printf("\nROOT\n");
//    fread(&entradaRoot1, sizeof(Tabent),1,disk);
//    printf("status: %x \n",entradaRoot1.status);
//    printf("tamanho: %ld \n",entradaRoot1.totalSize);
//    printf("setor: %d \n",entradaRoot1.sector);
//        printf("hora: %d \n",entradaRoot1.hora);
//        printf("min: %d \n",entradaRoot1.minuto);
//    printf("dia: %d \n",entradaRoot1.dia);
//    printf("mes: %d \n",entradaRoot1.mes);
//    printf("ano: %d \n",entradaRoot1.ano);
//    printf("setor: %u \n",entradaRoot1.sector);

    setRootNode(disk,boot);

    fseek(disk, 16+(boot.totalEntries*16), SEEK_SET);
    struct DataDirnode dirnode;
    fread(&dirnode, sizeof(struct DataDirnode),1,disk);
    printf("Status:%x  Ofsset%u\n",dirnode.staus,ftell(disk));
    printDirIndex(dirnode);
}

void searchFreeDataBlock(SAD16 sad16, unsigned int *sectors, int nsectors){

    unsigned int tablesize = sad16.boot.totalEntries*16;
    unsigned long int numofdatasectors = (sad16.boot.size/512) - ((tablesize +16)/512);

    fseek(sad16.disk, 16+(sad16.boot.totalEntries*16), SEEK_SET);
    Datanode data;
    int setoresencontrados=0;
    for(unsigned int i=0;i<numofdatasectors;i++){

        fread(&data, sizeof(data),1,sad16.disk);
//        printf("%u %d %lu\n",i,data.staus, ftell(sad16.disk));

        if((data.staus==0)&&(setoresencontrados<nsectors)){
            sectors[setoresencontrados]=i;
            setoresencontrados++;
        }else
            fseek(sad16.disk,512,SEEK_CUR);

        if(setoresencontrados==(nsectors-1))
            break;
    }
}

void createNewEntry(SAD16 sad16,char type, unsigned long int totalsize, unsigned int sector){

    time_t timer;
    time(&timer);
    struct tm *timeinfo;
    timeinfo = localtime (&timer);
    Tabent entrada;

    if(type=='F')
        entrada.status=15;
    if(type=='D')
        entrada.status=240;

    entrada.totalSize=totalsize;
    entrada.dia=(unsigned char)timeinfo->tm_mday;
    entrada.mes=(unsigned char)(timeinfo->tm_mon + 1);
    entrada.ano=(unsigned char)timeinfo->tm_year;
    entrada.sector=sector;

    unsigned int entry=0;
    for(unsigned int i=0;i<sad16.boot.totalEntries;i++){
        if(sad16.table[i].status==0){
            entry=i;
            break;
        }
    }
    if(entry!=0){
        fseek(sad16.disk,(16+(entry*16)),SEEK_SET);
        fwrite(&entrada, sizeof(Tabent),1,sad16.disk);
        printf("entrada Escrita em %u\n",entry);
    } else {
        printf("Não há entradas vazias\n");
    }
//    fclose(sad16.disk);
}

char *file_from_path (char *pathname) {
    char *fname = NULL;
    if (pathname) {
        fname = strrchr(pathname, '/') + 1;
    }
}

void insertFilledata(SAD16 sad16,FILE *fille,char *name, unsigned int *sectors,unsigned int nsector){
    printf("-----------------");
    Datanode data;

    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16)+(data.sector*512);

    for(unsigned int i=0;i<nsector;i++){
        fseek(sad16.disk,dataDesloc+(512)*sectors[i],SEEK_SET);
        fseek(fille,0, SEEK_SET);
        printf("%lu",ftell(sad16.disk));
        data.staus=15;
        data.sector=sectors[i+1];
        if(i=0){
            strcpy(data.data,name);
        } else{
            fread(&data.sector, sizeof(char), 507,fille);
        }
        if(i==nsector-1){
            data.sector=0;
        }
        fwrite(&data, sizeof(Datanode),1,sad16.disk);
        printf("dsadas");
    }

}
void printName(SAD16 sad16, unsigned int sectorHead){
    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16)+(sectorHead*512);
    Datanode data;
    fread(&data, sizeof(Datanode),1,sad16.disk);
//    printf("dsa%s",data.data[507]);
}


int main() {
    FILE *disk;
    FILE *test;
    disk=fopen("/dev/sdb1","wb+");
    test=fopen("/home/mhi/a.out","rb");
    if(!disk&&!test){
        printf("Não foi possivel abrir o disco");
    } else{
        format(disk);
        SAD16 sad16;
        sad16.disk=disk;
        fseek(disk,0,SEEK_SET);
        fread(&sad16.boot, sizeof(BootSAD),1,disk);
        Tabent tabent[sad16.boot.totalEntries];
        fread(tabent, sizeof(Tabent),sad16.boot.totalEntries,disk);
        sad16.table=tabent;
        fseek(test,0,SEEK_END);
        unsigned int sector=0;
        int nsector=ftell(test)/512;
        if((ftell(test)%512)!=0){
            nsector++;
        }
        unsigned int sectors[nsector];
        searchFreeDataBlock(sad16,sectors,nsector);
        createNewEntry(sad16,'F',ftell(test),sectors[0]);
        insertFilledata(sad16,test,file_from_path("/home/mhi/a.out"),sectors,ftell(test));
        fseek(disk,0,SEEK_END);
        printf("%lu",ftell(disk));
//        printName(sad16,1);
    }
    return 0;
}