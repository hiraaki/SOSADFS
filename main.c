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
    unsigned char data[507]; // Caso pirmeiro nodo da cadeia de byte é nome do arquivo, se não dado do arqivo
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
    for(int i=0;i<126;i++){
        datadirnode.indexTable[i]=0;
    }
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

    unsigned char blank[512];
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
//    printf("Nsectors %d\n",nsectors);
    for(unsigned int i=0;i<=numofdatasectors;i++){

        fread(&data, sizeof(data),1,sad16.disk);
//        printf("%u %d %lu\n",i,data.staus, ftell(sad16.disk));

        if((data.staus==0)&&(setoresencontrados<nsectors)){
            sectors[setoresencontrados]=i;
//            printf("%u %u ",i,setoresencontrados);
            setoresencontrados++;
        }

        if(setoresencontrados==nsectors)
            break;
    }
}

unsigned int createNewEntry(SAD16 sad16,char type, unsigned long int totalsize, unsigned int sector){

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
        printf("|i:%u status:%x| ",i,sad16.table[i].status);
        if(sad16.table[i].status==0){
            entry=i;
            break;
        }
    }
    if(entry!=0){
        fseek(sad16.disk,(16+(entry*16)),SEEK_SET);
        fwrite(&entrada, sizeof(Tabent),1,sad16.disk);
        sad16.table[entry]=entrada;
        printf("entrada Escrita em %u\n",entry);
        return entry;
    } else {
        printf("Não há entradas vazias\n");
        return 0;
    }

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

    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    printf("\n%lu\n",nsector);
    printf("\nsectors %d\n", sectors[nsector-1]);
    fseek(fille,0, SEEK_SET);
//    char dados[507];
//    fread(&dados, 1 , 507,fille);
    for(int i=0;i<507;i++){
        data.data[i]=0;
    }
    for(unsigned int i=0;i<nsector;i++){
        fseek(sad16.disk,dataDesloc+(sectors[i]*512),SEEK_SET);

        printf("%lu \n",ftell(sad16.disk));
        data.staus=15;
        data.sector=sectors[i+1];
        if(i==0){
            strcpy(data.data,name);
//            printf("%s\n",data.data);

        } else if(i==nsector-1){
            data.sector=0;
            for(int j=0;j<507;j++)
                data.data[j]=0;

            unsigned long int atual = ftell(fille);

            fseek(fille,0,SEEK_END);
            unsigned long int ultimo = ftell(fille);

            unsigned long int lastsize = ultimo-atual;

//            printf("\ntotal:%lu\n",ultimo);
//
//            printf("\nponteiro local:%lu\n",atual);
//
//            printf("\n ultimo pedaço:%lu\n",lastsize);
            fseek(fille,atual,SEEK_SET);

            fread(data.data, sizeof(char), lastsize ,fille);
            printf("\nstring de data %s %lu\n",data.data,lastsize);

        }else{
            fread(data.data, sizeof(char), 507,fille);
        }
        fwrite(&data, sizeof(Datanode),1,sad16.disk);
        printf("%u ",i);
    }

}
void printName(SAD16 sad16, unsigned int sectorHead){
    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16)+(sectorHead*512);
    Datanode data;
    fseek(sad16.disk,dataDesloc,SEEK_SET);
    fread(&data, sizeof(Datanode),1,sad16.disk);
    if(data.staus==240)
        printf("<Dir>\t");
    else
        printf("<File>\t");
    printf("%lu %s\n",dataDesloc,data.data);
}
void printEntry(Tabent tabent){

    printf("status: %x \n",tabent.status);
    printf("tamanho: %ld \n",tabent.totalSize);
    printf("setor: %d \n",tabent.sector);
    printf("dia: %d \n",tabent.dia);
    printf("mes: %d \n",tabent.mes);
    printf("ano: %d \n",tabent.ano);
    printf("setor: %u \n",tabent.sector);

}



void listdir(SAD16 sad16,unsigned int sector){
    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,dataDesloc+(sector*512),SEEK_SET);
    DataDirnode dataDirnode;
    fread(&dataDirnode, sizeof(dataDirnode),1,sad16.disk);
    int i=0;
    unsigned int sectorName = sad16.table[dataDirnode.indexTable[i]].sector;
    if(dataDirnode.indexTable[0]==0){
        printf("Diretório Vazio\n");
        return;
    } else{
        printf("\n(%d) ",i);
        printName(sad16,sectorName);
    }
    i++;
    while(dataDirnode.indexTable[i]!=0){
        printf("\n(%d) ",i);
        sectorName = sad16.table[dataDirnode.indexTable[i]].sector;
        printName(sad16,sectorName);
        i++;
    }
}


//cria entrada na tabela de entradas + alocação em data
unsigned int allocFille(SAD16 sad16, char filePath[]){
    FILE *file= fopen(filePath,"rb");
    unsigned int entry;
    if(file==NULL){
        printf("Não foi possivel abrir %s",filePath);
    } else{
        fseek(file,0,SEEK_END);
        unsigned int sector=0;
        unsigned long int nsector=(ftell(file)/507)+1;
        printf("%u",nsector);
        if((ftell(file)%512)!=0){
            nsector++;
        }
        unsigned int sectors[nsector];

        searchFreeDataBlock(sad16,sectors,nsector);
        entry=createNewEntry(sad16,'F',ftell(file),sectors[0]);

        insertFilledata(sad16,file,file_from_path(filePath),sectors,nsector);

        fclose(file);
    }
    return entry;
}

//MAX 126 entradas por diretório;
void createDirEntry(SAD16 sad16, unsigned int setorAtual, unsigned int index){
    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,dataDesloc+(setorAtual*512),SEEK_SET);
    DataDirnode dataDirnode;
    fread(&dataDirnode, sizeof(DataDirnode),1,sad16.disk);
    for(int i=0;i<126;i++){
        if(dataDirnode.indexTable[i]==0){
            dataDirnode.indexTable[i]=index;
            //reescrevendo o node com a entrada nova
            fseek(sad16.disk,dataDesloc+(setorAtual*512),SEEK_SET);
            fwrite(&dataDirnode, sizeof(DataDirnode),1,sad16.disk);
            break;
        }
    }
}

unsigned int createNewDir(SAD16 sad16,char *name){
    unsigned int sectors[2];
    DataDirnode dataDirnode;
    Datanode nameNode;
    searchFreeDataBlock(sad16,sectors,2);

    for(int i=0;i<507;i++){
        if(i<126){
            dataDirnode.indexTable[i]=0;
        }
        nameNode.data[i]=0;
    }

    dataDirnode.sector=0;
    dataDirnode.staus=240;

    nameNode.staus=240;
    nameNode.sector=sectors[1];
    stpcpy(nameNode.data,name);

    unsigned int index;

    index=createNewEntry(sad16,'D',0,sectors[0]);

    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,(dataDesloc+(sectors[0]*512)),SEEK_SET);
    fwrite(&nameNode, sizeof(Datanode),1,sad16.disk);

    fseek(sad16.disk,(dataDesloc+(sectors[1]*512)),SEEK_SET);
    fwrite(&dataDirnode, sizeof(DataDirnode),1,sad16.disk);

    return index;
}


unsigned int getQtdO(SAD16 sad16,unsigned int sector){

    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,dataDesloc+(sector*512),SEEK_SET);
    DataDirnode dataDirnode;
    fread(&dataDirnode, sizeof(dataDirnode),1,sad16.disk);

    unsigned int i=0;
    while(dataDirnode.indexTable[i]!=0)
        i++;

    return i;

}

void getListO(SAD16 sad16, unsigned int *list, unsigned int qtdo, unsigned int sector){
    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,dataDesloc+(sector*512),SEEK_SET);
    DataDirnode dataDirnode;
    fread(&dataDirnode, sizeof(dataDirnode),1,sad16.disk);

    unsigned int i=0;

    while(dataDirnode.indexTable[i]!=0){
//        printf("\n(%d) ",i);
        list[i] = sad16.table[dataDirnode.indexTable[i]].sector;
        i++;
    }

}


unsigned int getDirSector(SAD16 sad16,unsigned int sector){
    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,dataDesloc+(sector*512),SEEK_SET);
    Datanode datanode;
    fread(&datanode, sizeof(Datanode),1,sad16.disk);
    printf("\n%s\n",datanode.data);
    return datanode.sector;
}

void copyout(SAD16 sad16,Tabent tableEntry,char *path){

    printEntry(tableEntry);

    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,dataDesloc+(tableEntry.sector*512),SEEK_SET);
    unsigned int setores =  (tableEntry.totalSize/512)+1;
    unsigned long int ultimaparte =(tableEntry.totalSize%512);
    if(ultimaparte>0){
        setores++;
    }
    Datanode datanode;
    fread(&datanode, sizeof(Datanode),1,sad16.disk);

    char path2[507];
    strcat(path2,path);
    strcat(path2,datanode.data);

    printf("%s",path2);

    FILE *file = fopen(path2,"wb+");
    fseek(file,0,SEEK_SET);
//
    for(int i=0;i<setores-2;i++){
        printf("dsa");
        fseek(sad16.disk,dataDesloc+(datanode.sector*512),SEEK_SET);
        fread(&datanode, sizeof(Datanode),1,sad16.disk);
        fwrite(datanode.data, sizeof(char), sizeof(datanode.data),file);
    }
    fseek(sad16.disk,dataDesloc+(datanode.sector*512),SEEK_SET);
    fread(&datanode, sizeof(Datanode),1,sad16.disk);
    fwrite(datanode.data, sizeof(char), ultimaparte,file);
    fclose(file);
}


void getListofTE(SAD16 sad16, Tabent *list, unsigned int qtdo, unsigned int setorAtual){
    unsigned long int dataDesloc = 16+(sad16.boot.totalEntries*16);
    fseek(sad16.disk,dataDesloc+(setorAtual*512),SEEK_SET);
    DataDirnode dataDirnode;
    fread(&dataDirnode, sizeof(dataDirnode),1,sad16.disk);

    unsigned int i=0;

    while(dataDirnode.indexTable[i]!=0){
//        printf("\n(%d) ",i);
        list[i] = sad16.table[dataDirnode.indexTable[i]];
        i++;
    }

}

int main() {
    FILE *disk;
    char diskPath[]="/home/mhi/disco";
    char filePath[]="/home/mhi/alfa.txt";
    char exitPath[]="/home/mhi/saida/";
    unsigned int setorAtual=0;
    unsigned int setorAterior=0;

    disk=fopen("/home/mhi/disco","rb+");
    if(disk==NULL){
        printf("Não foi possivel abrir o disco");
    } else{
        int op;
        SAD16 sad16;
        sad16.disk=disk;
        fseek(disk,0,SEEK_SET);
        fread(&sad16.boot, sizeof(BootSAD),1,disk);
        printBoot(sad16.boot);
        Tabent tabent[sad16.boot.totalEntries];
        fread(tabent, sizeof(Tabent),sad16.boot.totalEntries,disk);
        sad16.table=tabent;



        while(scanf("%d",&op)!=0) {

            if (op == 1) {
                sad16.disk = fopen(diskPath, "wb+");
                format(sad16.disk);

            } else if (op == 2) {
                sad16.disk = fopen(diskPath, "rb+");
                unsigned int entry;
                entry = allocFille(sad16, filePath);
                printEntry(tabent[entry]);
                createDirEntry(sad16, setorAtual, entry);

            } else if (op == 3) {
                sad16.disk = fopen(diskPath, "rb");
                listdir(sad16, setorAtual);


            } else if (op == 4) {
                sad16.disk = fopen(diskPath, "rb+");
                char name[507];
                scanf("%s", &name);
                unsigned int index;
                index=createNewDir(sad16, name);
                createDirEntry(sad16,setorAtual,index);

            } else if(op==5){
                sad16.disk = fopen(diskPath, "rb");
                unsigned int selected;
                listdir(sad16,setorAtual);
                scanf("%u",&selected);

                unsigned int qtde= getQtdO(sad16,setorAtual);
                Tabent listTE[qtde];
                getListofTE(sad16,listTE,qtde,setorAtual);
                copyout(sad16,listTE[selected],exitPath);
                printf("pronto");

            } else if(op==6) {
                sad16.disk = fopen(diskPath, "rb");
                unsigned int qtdo = getQtdO(sad16,setorAtual);
                unsigned int setoresNodiretorio[qtdo];
                unsigned int selected;
                for(int i=0;i<qtdo;i++)
                    setoresNodiretorio[i]=0;

                qtdo = getQtdO(sad16,setorAtual);
                printf("%u",qtdo);
                getListO(sad16,setoresNodiretorio,qtdo,setorAtual);
                for(int i=0;i<qtdo;i++)
                    printf("%u ",setoresNodiretorio[i]);
                listdir(sad16,setorAtual);
                scanf("%d",&selected);
                selected = setoresNodiretorio[selected];
                setorAtual=getDirSector(sad16,selected);
                printf("\n%u\n",setorAtual);
            } else if(op==7){
                setorAtual = setorAterior;
            }else if (op == 8) {
//                unsigned int sectors[34];
//                searchFreeDataBlock(sad16, sectors, 34);
                for(int i=0; i<sad16.boot.totalEntries;i++)
                    printf("|%d|",sad16.table[i].status);
            }

            fclose(sad16.disk);
        }





    }
    return 0;
}