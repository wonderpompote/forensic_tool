# Forensic Tool
*Group assignment for the Data Forensics module (University of Limerick)*

Forensic tool to identify the disk partitions and volume information of a given disk image.

**Partitions information:** 
- number of partition, 
- presence of hidden partitions,
- for each partition: 
  - start sector, 
  - end sector,
  - size, 
  - type

**FAT volume information:**
- number of sectors per cluster, 
- size of the FAT area, 
- size of the Root Directory in sectors, 
- cluster 2 sector address, 
- or each deleted file: 
  - name, 
  - starting cluster number, 
  - size, 
  - start sector address, 
  - first 16 bytes

**NTFS volume information:**
- number of bytes per sector, 
- number of sectors per cluster, 
- logical cluster number for the MFT file record, 
- MFT file record sector address, 
- information about the first two attributes of a file: 
  - start offset, 
  - type (id and name), 
  - length
