#include <stdio.h>
#include <time.h>

typedef struct SAD16{

}__attribute__((packed)) SAD16;


typedef struct BootSAD{
        unsigned short int sectorSize;
        unsigned int totalEntries; // dado em setores, cada setor 512 bytes
        unsigned char entrySize;
        unsigned char errorForm;
        unsigned int errorHead;
        unsigned int errorSector;
}__attribute__((packed)) BootSAD;


typedef struct Tabent{
    char status; // 4bits altos arquivo/diretório, 4 bits baixos (uso ou vazio)
    struct totalSize totalSize1; // tamanho total da pasta ou do arquivo
    short int losize; //
    unsigned int sector; //Setor do cabeçalho do arquivo
    char hora; // Hora da criação/escrita do arquvo/diretório
    char minuto;
    char dia; // Data da criação/escrita do arquvo/diretório
    char mes;
}__attribute__((packed)) Tabent;


void setBoot(FILE *disk, unsigned numbSectors){
    fseek(disk,0,SEEK_SET);
    BootSAD boot;
    boot.sectorSize=512;
    boot.totalEntries=(numbSectors*0.05)+1;
    boot.entrySize=16;
    boot.errorForm=1;
    boot.errorHead=0;
    boot.errorSector=0;
    fwrite(&boot,sizeof(BootSAD),1,disk);
}
void setRootEntry(){
    time_t rawtime;
    struct tm * timeinfo;
    time (&rawtime);
    timeinfo = localtime (&rawtime);
    Tabent entradaRoot;
    entradaRoot.status=240;
    entradaRoot.totalSize1=1;
    entradaRoot.sector=1; //primeiro setor da área de dados é o root
    entradaRoot.hora=timeinfo->tm_hour;
    entradaRoot.minuto=timeinfo->tm_min;
    entradaRoot.dia=timeinfo->tm_mday;
    entradaRoot.mes=timeinfo->tm_mon + 1;
}



int main() {
    FILE *disk;
    disk=fopen("/dev/sde1","rb+");
    if(!disk){
        printf("Não foi possivel abrir o disco");
    } else{
        setBoot(disk,2048);
        fseek(disk,0,SEEK_SET);
        BootSAD boot;
        fread(&boot, sizeof(BootSAD),1,disk);
        printf("%d \n", boot.sectorSize);
        printf("%d \n", boot.totalEntries);
        printf("%d \n", boot.entrySize);
        printf("%d \n", boot.errorForm);
        printf("%d \n", boot.errorHead);
        printf("%d \n", boot.errorSector);
    }
    return 0;
}