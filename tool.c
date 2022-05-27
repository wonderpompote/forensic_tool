/**
 * @file tool.c
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NB_PARTITION 4
#define SECTOR_SIZE 512 

struct Partition{ 
    char type; 
    unsigned int start_sect;
    unsigned int last_sector; 
    unsigned int size;
} part_entry[MAX_NB_PARTITION]; // 4 x partition table entry

struct Hidden_partition{
    int start_position;
    int stop_position;
    int size;
} hidden_part[MAX_NB_PARTITION-1]; // 3 x potential hidden partition

// function to display the total number of partitions
void partitions_info(FILE *fp, int offset){
    int not_exist = 0;
    char vol_type[20];
    unsigned char buf_part_table[64];
    fseek(fp, 0x1BE, SEEK_SET); // Seek to the start of the part_entry list
    fread(buf_part_table, 1, 64, fp); // Read the 64-byte block to memory buffer
    printf("\n-------------------------------------------------------------------------------------");
    // for each potential partition
    for (int i=0; i < MAX_NB_PARTITION; i++) {
        part_entry[i].type = *(char*)(buf_part_table + 0x04 +(i * offset) ); // type of partition
        if (part_entry[i].type == 0){
            not_exist++;
        }
        part_entry[i].start_sect = *(int*)(buf_part_table + 0x08 +(i * offset)); // start sector
        part_entry[i].size = *(int*)(buf_part_table + 0x0C + (i * offset)) ; // size of partition
        part_entry[i].last_sector = (part_entry[i].start_sect) + (part_entry[i].size) - 1; // last sector of partition
        // volume type name
        switch (part_entry[i].type) {
            case 00 : 
                strcpy (vol_type, "NOT-VALID");
                part_entry[i].last_sector = 0;
                break;
            case 04 : 
                strcpy (vol_type, "16-bit FAT"); 
                break;
            case 05 : 
                strcpy (vol_type, "Ex MS-DOS"); 
                break;
            case 06 : 
                strcpy (vol_type, "FAT-16"); 
                break;
            case 07 : 
                strcpy (vol_type, "NTFS");
                break;
            case 0x0B: 
                strcpy (vol_type, "FAT-32"); 
                break;
            case 0x0C: 
                strcpy (vol_type, "FAT-32 (LBA)"); 
                break;
            case 0x0E:
                strcpy (vol_type, "FAT-16 (LBA)");
                break;
            default: 
                strcpy (vol_type, "NOT-DECODED"); 
                break;
        }
        // Print out partition content
        printf ("\nPartition %d: Type: %-12s Start: %-12d Size: %-12d Last: %-12d", i, vol_type, part_entry[i].start_sect, part_entry[i].size, part_entry[i].last_sector);
    }
    printf("\n-------------------------------------------------------------------------------------");
    printf("\nTotal number of valid partitions: %d\n", MAX_NB_PARTITION-not_exist);
}

// function to display the hidden partitions
void hidden_partition_information(){
    int nb_hidden_partitions = 0;
    // check for hidden partitions
    for (int i=0; i<MAX_NB_PARTITION; i++){
        if ((i != 0 && i<3) && (part_entry[i].type != 00)){ // check if hidden partition for valid partitions
            // if last sector of previous partition+1 != start sector of partition --> hidden partition
            if (part_entry[i].start_sect != part_entry[i-1].last_sector+1){ 
                if (nb_hidden_partitions == 0) printf("\n-------------------------------------------------------------------------------------");
                hidden_part[nb_hidden_partitions].start_position = (i-1);
                hidden_part[nb_hidden_partitions].stop_position = i;
                hidden_part[nb_hidden_partitions].size = part_entry[i].start_sect - part_entry[i-1].last_sector;
                printf("\nHidden Partition between partition: %d and partition: %d \t\tSize: %-12d", hidden_part[nb_hidden_partitions].start_position, hidden_part[nb_hidden_partitions].stop_position, hidden_part[nb_hidden_partitions].size);
                nb_hidden_partitions++;
            }
        }
    }
    if (nb_hidden_partitions > 0) printf("\n-------------------------------------------------------------------------------------");
    printf("\nTotal number of hidden partitions: %d\n", nb_hidden_partitions);
}

// function to display information about FAT_16 partition
void information_FAT_16_part(FILE *fp, int index){
    unsigned char buf_part_FAT_16[64]; // buffer for FAT area

    fseek(fp, part_entry[index].start_sect*SECTOR_SIZE, SEEK_SET); // Seek to the start of the first partition
    fread(buf_part_FAT_16, 1, 64, fp); // Read the 64-byte block to memory buffer

    printf("\n\n -------- Description of FAT-16 partition ---------");
    // number of sectors
    unsigned char nb_sector = *(char*)(buf_part_FAT_16 + 0x0D);
    printf("\n- Number of sectors per cluster: %d", nb_sector);
    // size of FAT area
    unsigned char nb_copy = *(char*)(buf_part_FAT_16 + 0x10);
    unsigned short int fat_size = *(short int*)(buf_part_FAT_16 + 0x16);
    printf("\n- Size of FAT Area: %d sectors", nb_copy*fat_size);
    // root directory size
    unsigned short int max_nb_root_dir = *(short int*)(buf_part_FAT_16 + 0x11);
    int root_dir_size = (max_nb_root_dir*32)/SECTOR_SIZE;
    printf("\n- Size of Root Directory in sectors: %d", root_dir_size);
    // sector address of cluster 2 (first cluster of data area)
    unsigned short int res_area_size = *(short int*)(buf_part_FAT_16 + 0x0E);
    int sector_addr_data_area = part_entry[0].start_sect + res_area_size + nb_copy*fat_size;
    int sector_addr_cluster_2 = sector_addr_data_area+root_dir_size;
    printf("\n- Sector address of cluster 2: %d",sector_addr_cluster_2);

    unsigned char buf_root_dir[SECTOR_SIZE]; // buffer for root directory
    fseek(fp, sector_addr_data_area*SECTOR_SIZE, SEEK_SET); // Seek to the start of the root directory
    fread(buf_root_dir, 1, SECTOR_SIZE, fp); // Read the 256-byte block to memory buffer
    
    printf("\n--------- Description of deleted file(s) ----------");
    int nb_deleted_files = 0;
    for (int i=0; i<SECTOR_SIZE; i +=32){ // look through the whole sector
        if(buf_root_dir[i] == 0xE5){ // if file was deleted
            // print name of the file
            printf("\nThe file '");
            for(int j=0;j<11;j++){
                printf("%c",buf_root_dir[i+j]);
            }
            printf("' has been deleted");
            // cluster number
            unsigned short int cluster_number = *(short int*)(buf_root_dir+ i + 0x1A);
            if (cluster_number != 0){ // if starting cluster address = 0 --> file empty
                printf("\n- Starting cluster number of the file: %d",cluster_number);
                unsigned int file_size = *(int*)(buf_root_dir+ i + 0x1C);
                printf("\n- Size of deleted file: %d bytes",file_size);

                int cluster_sector_addr = sector_addr_cluster_2 + ((cluster_number - 2)*8);
                unsigned char buf_deleted_file[16];
                printf("\n- Start Sector address: %d", cluster_sector_addr);

                fseek(fp, cluster_sector_addr*SECTOR_SIZE, SEEK_SET); // Seek to the start of the first deleted file
                fread(buf_deleted_file, 1, 16, fp); // Read the 16-byte block to memory buffer

                printf("\n- First 16 bytes of the deleted file: \n'");
                for(int k=0;k<16;k++){
                    printf("%c",buf_deleted_file[k]);
                }
                printf("'");
            }
            else { // starting cluster address = 0
                printf("\n- Empty file\n");
            }
            nb_deleted_files ++;
        }
    }
    printf("\n\nTotal number of deleted files: %d",nb_deleted_files);
}

void information_NTFS_part(FILE *fp, int partition_number){
    printf("\nPartition %d:",partition_number);
    unsigned char buffer_ntfs_part_table[64];
    fseek(fp, part_entry[partition_number].start_sect*SECTOR_SIZE, SEEK_SET); // Seek the start of the NTFS partition
    fread(buffer_ntfs_part_table, 1, 64, fp); // Read the 64-byte block to memory buffer

    // read number of bytes per sector
    short unsigned int bytes_per_sector = *(int*)(buffer_ntfs_part_table + 0x0B);
    printf("\n- Number of bytes per sector: %d", bytes_per_sector);
    // read number of sectors per cluster
    unsigned char sectors_per_cluster = *(char*)(buffer_ntfs_part_table + 0x0D);
    printf("\n- Number of sectors per cluster: %d", sectors_per_cluster);
    // read logical cluster number for $MFT file record
    unsigned long int MFT_cluster_number = *(long int*)(buffer_ntfs_part_table + 0x30);
    printf("\n- Logical cluster number for MFT file record: %ld", MFT_cluster_number);
    long int MFT_sector_address = (sectors_per_cluster * MFT_cluster_number) + part_entry[partition_number].start_sect;
    printf("\n- MFT file record sector address: %ld",MFT_sector_address);

    unsigned char buf_MFT_record[1024]; // buffer for basic MFT record (1024 bytes)
    
    fseek(fp, MFT_sector_address*SECTOR_SIZE, SEEK_SET); // Seek to the start of the MFT record
    fread(buf_MFT_record, 1, 1024, fp); // Read the 1024-byte block to memory buffer

    // first attribute offset
    unsigned short int offset = *(short int*)(buf_MFT_record + 0x14);
    printf("\n----------------- NTFS ATTRIBUTES ------------------");
    for(int j=1; j<3; j++){
        printf("\nAttribute %d",j);
        printf("\n- offset: 0x%x", offset);
        unsigned long int attribute_type = *(long int*)(buf_MFT_record + offset);
        char attribute_type_name[60];
        switch (attribute_type) {
            case 16 : 
                strcpy (attribute_type_name, "$STANDARD_INFORMATION");
                break;
            case 32 : 
                strcpy (attribute_type_name, "$ATTRIBUTE_LIST"); 
                break;
            case 48 : 
                strcpy (attribute_type_name, "$FILE_NAME"); 
                break;
            case 64 : 
                strcpy (attribute_type_name, "$OBJECT_ID"); 
                break;
            case 80 : 
                strcpy (attribute_type_name, "$SECURITY_DESCRIPTOR"); 
                break;
            case 96 : 
                strcpy (attribute_type_name, "$VOLUME_NAME"); 
                break;
            case 122 : 
                strcpy (attribute_type_name, "$VOLUME_INFORMATION"); 
                break;
            case 128 : 
                strcpy (attribute_type_name, "$DATA"); 
                break;
            case 144 : 
                strcpy (attribute_type_name, "$INDEX_ROOT"); 
                break;	   	
            case 160 : 
                strcpy (attribute_type_name, "$INDEX_ALLOCATION"); 
                break;
            case 176 : 
                strcpy (attribute_type_name, "$BITMAP"); 
                break;                		
            case 192 : 
                strcpy (attribute_type_name, "$REPARSE_POINT"); 
                break;
            case 256 : 
                strcpy (attribute_type_name, "$LOGGED_UTILITY_STREAM"); 
                break;
        }
        printf("\n- type: %ld - %s",attribute_type,attribute_type_name);
        unsigned long int attribute_length = *(long int*)(buf_MFT_record + offset + 0x04);
        printf("\n- length: %ld",attribute_length);
        offset += attribute_length;
        printf("\n----------------------------------------------------");
    }
}

int main(int argc, char *argv[])
{
// Define the variables
    int offset = 16;
    FILE *fp;
    
    fp = fopen(argv[1], "rb"); // Open file for reading - binary mode. Should use error check!

    printf("\n\t\t\t\tForensic assignment\n");
    printf("\nPartitions information:");

    // Print information about the partitions
    partitions_info(fp, offset);
    // print information about hidden partitions if any
    hidden_partition_information();

     // check if first partition is FAT-16 (should be)
    if(part_entry[0].type == 06 || part_entry[0].type == 0x0E){
        fp = fopen(argv[1], "rb"); // Open file for reading - binary mode. Should use error check!
        information_FAT_16_part(fp, 0); // read info first FAT partition
    }

    // description of NTFS partition(s)
    printf("\n\n--------- Description of NTFS partition(s) ---------");
    for(int i=0; i<MAX_NB_PARTITION; i++){
        if(part_entry[i].type == 07){ // if partition is NTFS
            fp = fopen(argv[1], "rb"); // Open file for reading - binary mode. Should use error check!
            information_NTFS_part(fp, i);
        }
    }

    fclose(fp);
    return(0);
}
