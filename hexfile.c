/**
   \file hexfile.c

   \author G. Icking-Konert
   \date 2018-12-14
   \version 0.2

   \brief implementation of routines for HEX, S19 and table files

   implementation of routines for importing and exporting Motorola S19 and Intel HEX files,
   as well as plain ASCII tables.
   (format descriptions under http://en.wikipedia.org/wiki/SREC_(file_format) or
   http://www.keil.com/support/docs/1584.htm).
*/

// include files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "hexfile.h"
#include "console.h"
#include "main.h"


/// last error in module hexfile
STM8gal_HexFileErrors_t g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

char * g_hexFileErrorStrings[STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER+1] = 
{ 
    "No Error",                             // STM8GAL_HEXFILE_NO_ERROR = 0,
    "Failed to open file",                  // STM8GAL_HEXFILE_FAILED_OPEN_FILE,
    "Failed to create file",                // STM8GAL_HEXFILE_FAILED_CREATE_FILE,
    "File buffer size exceeded",            // STM8GAL_HEXFILE_FILE_BUFFER_SIZE_EXCEEDED,
    "S record invalid start",               // STM8GAL_HEXFILE_S_RECORD_INVALID_START,
    "S record address buffer exceeded",     // STM8GAL_HEXFILE_S_RECORD_ADDRESS_BUFFER_EXCEEDED,
    "S record checksum error",              // STM8GAL_HEXFILE_S_RECORD_CHKSUM_ERROR,
    "Hex file invalid start",               // STM8GAL_HEXFILE_HEX_FILE_INVALID_START,
    "Hex file address buffer exceeded",     // STM8GAL_HEXFILE_HEX_FILE_ADDRESS_BUFFER_EXCEEDED,
    "Hex file address exceeded segment",    // STM8GAL_HEXFILE_HEX_FILE_ADDRESS_EXCEEDED_SEGMENT,
    "Hex file unsupported record type",     // STM8GAL_HEXFILE_HEX_FILE_UNSUPPORTED_RECORD_TYPE,
    "Hex file Checksum error",              // STM8GAL_HEXFILE_HEX_FILE_CHKSUM_ERROR,
    "Invalid character",                    // STM8GAL_HEXFILE_INVALID_CHAR,
    "File address invalid",                 // STM8GAL_HEXFILE_FILE_ADDRESS_INVALID,
    "File address exceeds buffer",          // STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER,
};

/**
   \fn char *get_line(char **buf, char *line)

   \param[in]  buf        pointer to read from (is updated)
   \param[out] line       pointer to line read (has to be large anough)

   read line (until LF, CR, or EOF) from RAM buffer and advance buffer pointer.
   memory for line has to be allocated externally
*/
char *get_line(char **buf, char *line) {

  char  *p = line;

  // copy line
  while ((**buf!=10) && (**buf!=13) && (**buf!=0)) {
    *line = **buf;
    line++;
    (*buf)++;
  }

  // skip CR + LF in buffer
  while ((**buf==10) || (**buf==13))
    (*buf)++;

  // terminate line
  *line = '\0';

  // check if data was copied
  if (p == line)
    return(NULL);
  else
    return(p);

} // get_line



/**
   \fn STM8gal_HexFileErrors_t hexfile_loadFile(const char *filename, char *fileBuf, uint64_t *lenFileBuf, uint8_t verbose)

   \param[in]  filename     name of file to read
   \param[out] fileBuf      memory buffer containing file content
   \param[out] lenFileBuf   size of data [B] read from file
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

  read file from file to memory buffer. Don't interpret (is done in separate routine)
*/
STM8gal_HexFileErrors_t hexfile_loadFile(const char *filename, char *fileBuf, uint64_t *lenFileBuf, uint8_t verbose) {

  FILE      *fp;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // strip path from filename for readability
  #if defined(WIN32) || defined(WIN64)
    const char *shortname = strrchr(filename, '\\');
  #else
    const char *shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose >= SILENT)
    console_print(STDOUT, "  load '%s' ... ", shortname);

  // open file to read
  if (!(fp = fopen(filename, "rb"))) {
    g_hexFileErrors = STM8GAL_HEXFILE_FAILED_OPEN_FILE;
    console_print(STDERR, "Failed to open file %s", filename);
    return(g_hexFileErrors);
  }

  // get filesize
  fseek(fp, 0, SEEK_END);
  (*lenFileBuf) = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // check file size vs. buffer
  if ((*lenFileBuf) > LENFILEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_BUFFER_SIZE_EXCEEDED;
    console_print(STDERR, "File %s exceeded buffer size (%ld vs %ld)", (*lenFileBuf), LENFILEBUF);
    return(g_hexFileErrors);
  }

  // init memory image to zero
  memset(fileBuf, 0, LENFILEBUF * sizeof(*fileBuf));

  // read file to buffer
  fread(fileBuf, (*lenFileBuf), 1, fp);

  // close file again
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)){
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if ((*lenFileBuf)>1024*1024)
      console_print(STDOUT, "done (%1.1fMB)\n", (float) (*lenFileBuf)/1024.0/1024.0);
    else if ((*lenFileBuf)>1024)
      console_print(STDOUT, "done (%1.1fkB)\n", (float) (*lenFileBuf)/1024.0);
    else if ((*lenFileBuf)>0)
      console_print(STDOUT, "done (%dB)\n", (int) (*lenFileBuf));
    else
      console_print(STDOUT, "done, no data read\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_loadFile



/**
   \fn STM8gal_HexFileErrors_t hexfile_convertS19(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  fileBuf      memory buffer to read from
   \param[in]  lenFileBuf   length of memory buffer
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

   convert memory buffer containing s19 hexfile to memory image. For description of
   Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
STM8gal_HexFileErrors_t hexfile_convertS19(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose) {

  char      line[1000], tmp[1000], *p;
  uint64_t  linecount, idx;
  uint8_t   type, len, chkRead, chkCalc;
  uint64_t  addr, addrStart, addrStop, numData;
  int       val, i;

  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  convert S19 ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  convert Motorola S19 file ... ");


  //////
  // import data to memory with syntax check
  //////
  p          = fileBuf;
  linecount  = 0;
  numData    = 0;
  addrStart  = 0xFFFFFFFFFFFFFFFF;
  addrStop   = 0x0000000000000000;
  while ((uint64_t) (p-fileBuf) < lenFileBuf) {

    // get next line. On EOF terminate
    if (!get_line(&p, line))
      break;

    // increase line counter
    linecount++;
    chkCalc = 0x00;

    // check 1st char (must be 'S')
    if (line[0] != 'S') {
      g_hexFileErrors = STM8GAL_HEXFILE_S_RECORD_INVALID_START;
      console_print(STDERR, "Line %u in Motorola S-record file: line does not start with 'S'", linecount);
      return(g_hexFileErrors);
    }

    // record type
    type = line[1]-48;

    // skip if line contains no data, i.e. line doesn't start with S1, S2 or S3
    if ((type != 1) && (type != 2) && (type != 3))
      continue;

    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+2, 2);
    sscanf(tmp, "%x", &val);
    len = val;
    chkCalc += val;              // increase checksum

    // address (S1=16bit, S2=24bit, S3=32bit)
    addr = 0;
    for (i=0; i<type+1; i++) {
	  sprintf(tmp,"0x00");
      tmp[2] = line[4+(i*2)];
      tmp[3] = line[5+(i*2)];
      sscanf(tmp, "%x", &val);
      addr *= (uint64_t) 256;
      addr += (uint64_t) val;
      chkCalc += (uint8_t) val;
    }

    // check for buffer overflow
    if (addr > (uint64_t) (LENIMAGEBUF-1L)) {
      g_hexFileErrors = STM8GAL_HEXFILE_S_RECORD_ADDRESS_BUFFER_EXCEEDED;
      console_print(STDERR, "Line %u in Motorola S-record file: buffer address exceeded (%dMB vs %dMB)", linecount, (int) (addr/1024L/1024L), (int) (LENIMAGEBUF/1024L/1024L));
      return(g_hexFileErrors);
    }

    // read record data
    idx=6+(type*2);                     // start at position 8, 10, or 12, depending on record type
    len=len-1-(1+type);                 // substract chk and address length
    for (i=0; i<len; i++) {
      sprintf(tmp,"0x00");
      strncpy(tmp+2, line+idx, 2);      // get next 2 chars as string
      sscanf(tmp, "%x", &val);          // interpret as hex data
      imageBuf[addr+i] = ((uint16_t) val) | 0xFF00;  // store data byte in buffer and set high byte for "defined"
      numData++;                        // increade byte counter
      chkCalc += (uint8_t) val;                   // increase checksum
      idx+=2;                           // advance 2 chars in line
    }

    // for printout store min/max address in file
    if (addr       < addrStart)  addrStart = addr;
    if (addr+len-1 > addrStop)   addrStop  = addr+len-1;

    // checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &val);
    chkRead = (uint8_t) val;

    // assert checksum (0xFF xor (sum over all except record type)
    chkCalc ^= 0xFF;                 // invert checksum
    if (chkCalc != chkRead) {
      g_hexFileErrors = STM8GAL_HEXFILE_S_RECORD_CHKSUM_ERROR;
      console_print(STDERR, "Line %u in Motorola S-record file: checksum error (0x%02x vs. 0x%02x)", linecount, chkRead, chkCalc);
      return(g_hexFileErrors);
    }

  } // while !EOF

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0/1024.0, addrStart, addrStop);
    else if (numData>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0, addrStart, addrStop);
    else if (numData>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_convertS19



/**
   \fn STM8gal_HexFileErrors_t hexfile_convertIHex(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  fileBuf      memory buffer to read from
   \param[in]  lenFileBuf   length of memory buffer
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

   convert memory buffer containing intel hexfile to memory buffer. For description of
   Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/
STM8gal_HexFileErrors_t hexfile_convertIHex(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose) {

  char      line[1000], tmp[1000], *p;
  uint64_t  linecount, idx;
  uint8_t   type, len, chkRead, chkCalc;
  uint64_t  addr, addrStart, addrStop, numData;
  uint64_t  addrOffset, addrJumpStart;
  int       val, i;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // avoid compiler warning (variable not yet used). See https://stackoverflow.com/questions/3599160/unused-parameter-warnings-in-c
  (void) (addrJumpStart);

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  convert IHX ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  convert Intel HEX file ... ");


  //////
  // import data to memory with syntax check
  //////
  p          = fileBuf;
  linecount  = 0;
  numData    = 0;
  addrStart  = 0xFFFFFFFFFFFFFFFF;
  addrStop   = 0x0000000000000000;
  addrOffset = 0x0000000000000000;
  while ((uint64_t) (p-fileBuf) < lenFileBuf) {

    // get next line. On EOF terminate
    if (!get_line(&p, line))
      break;

    // increase line counter
    linecount++;
    chkCalc = 0x00;

    // check 1st char (must be ':')
    if (line[0] != ':') {
      g_hexFileErrors = STM8GAL_HEXFILE_HEX_FILE_INVALID_START;
      console_print(STDERR, "Line %u in Intel hex file: line does not start with ':'", linecount);
      return(g_hexFileErrors);
    }

    // record length (address + data + checksum)
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+1, 2);
    sscanf(tmp, "%x", &val);
    len = val;
    chkCalc += len;              // increase checksum

    // 16b address
    addr = 0;
    sprintf(tmp,"0x0000");
    strncpy(tmp+2, line+3, 4);
    sscanf(tmp, "%x", &val);
    chkCalc += (uint8_t) (val >> 8);
    chkCalc += (uint8_t)  val;
    addr = (uint64_t) (val + addrOffset);	   // add offset for >64kB addresses

    // record type
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+7, 2);
    sscanf(tmp, "%x", &val);
    type = val;
    chkCalc += type;              // increase checksum

    // record contains data
    if (type==0) {

      // check for buffer overflow
      if (addr > (uint64_t) (LENIMAGEBUF-1L)) {
        g_hexFileErrors = STM8GAL_HEXFILE_HEX_FILE_ADDRESS_BUFFER_EXCEEDED;
        console_print(STDERR, "Line %u in Intel hex file: buffer size exceeded (%dMB vs %dMB)", linecount, (int) (addr/1024L/1024L), (int) (LENIMAGEBUF/1024L/1024L));
        return(g_hexFileErrors);
      }

      // for printout store min/max address in file
      if (addr < addrStart)  addrStart = addr;
      if (addr > addrStop)   addrStop  = addr;

      // get data
      idx = 9;                            // start at index 9
      for (i=0; i<len; i++) {
        sprintf(tmp,"0x00");
        strncpy(tmp+2, line+idx, 2);      // get next 2 chars as string
        sscanf(tmp, "%x", &val);          // interpret as hex data
        imageBuf[addr+i] = val | 0xFF00;  // store data byte in buffer and set high byte for "defined"
        numData++;                        // increade byte counter
        chkCalc += val;                   // increase checksum
        idx+=2;                           // advance 2 chars in line
      }

    } // type==0

    // EOF indicator
    else if (type==1)
      continue;

    // extended segment addresses not yet supported
    else if (type==2) {
      g_hexFileErrors = STM8GAL_HEXFILE_HEX_FILE_ADDRESS_EXCEEDED_SEGMENT;
      console_print(STDERR, "Line %u in Intel hex file: extended segment address type 2 not supported", linecount);
      return(g_hexFileErrors);
    }

    // start segment address (only relevant for 80x86 processor, ignore here)
    else if (type==3)
      continue;

    // extended address (=upper 16b of address for following data records)
    else if (type==4) {
      idx = 13;                       // start at index 13
      sprintf(tmp,"0x0000");
      strncpy(tmp+2, line+9, 4);      // get next 4 chars as string
      sscanf(tmp, "%x", &val);        // interpret as hex data
      chkCalc += (uint8_t) (val >> 8);
      chkCalc += (uint8_t)  val;
      addrOffset = ((uint64_t) val) << 16;
    } // type==4

    // start linear address records. Can be ignored, see http://www.keil.com/support/docs/1584/
    else if (type==5)
      continue;

    // unsupported record type -> error
    else {
      g_hexFileErrors = STM8GAL_HEXFILE_HEX_FILE_UNSUPPORTED_RECORD_TYPE;
      console_print(STDERR, "Line %u in Intel hex file: unsupported type %d", linecount, type);
      return(g_hexFileErrors);
    }


    // checksum
    sprintf(tmp,"0x00");
    strncpy(tmp+2, line+idx, 2);
    sscanf(tmp, "%x", &val);
    chkRead = val;

    // assert checksum (0xFF xor (sum over all except record type))
    chkCalc = 255 - chkCalc + 1;                 // calculate 2-complement
    if (chkCalc != chkRead) {
      g_hexFileErrors = STM8GAL_HEXFILE_HEX_FILE_CHKSUM_ERROR;
      console_print(STDERR, "Line %u in Intel hex file: checksum error (read 0x%02x, calc 0x%02x)", linecount, chkRead, chkCalc);
      return(g_hexFileErrors);
    }

  } // while !EOF

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0/1024.0, addrStart, addrStop);
    else if (numData>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0, addrStart, addrStop);
    else if (numData>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_convertIHex



/**
   \fn STM8gal_HexFileErrors_t hexfile_convertTxt(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  fileBuf      memory buffer to read from
   \param[in]  lenFileBuf   length of memory buffer
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

   convert memory buffer containing plain table (address / value) to memory buffer.
   Address and value may be decimal (plain numberst) or hexadecimal (starting with '0x').
   Lines starting with '#' are ignored. No syntax check is performed.
*/
STM8gal_HexFileErrors_t hexfile_convertTxt(char *fileBuf, uint64_t lenFileBuf, uint16_t *imageBuf, uint8_t verbose) {

  char      line[1000], *p;
  uint64_t  linecount;
  char      sAddr[1000], sValue[1000];
  uint64_t  addr, addrStart, addrStop, numData;
  int       val, i;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  convert table ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  convert ASCII table file ... ");


  //////
  // import data to memory with syntax check
  //////
  p          = fileBuf;
  linecount  = 0;
  numData    = 0;
  addrStart  = 0xFFFFFFFFFFFFFFFF;
  addrStop   = 0x0000000000000000;
  while ((uint64_t) (p-fileBuf) < lenFileBuf) {

    // get next line. On EOF terminate
    if (!get_line(&p, line))
      break;

    // increase line counter
    linecount++;

    // if line starts with '#' ignore as comment
    if (line[0] == '#')
      continue;

    // get address and value as string
    sscanf(line, "%s %s", sAddr, sValue);


    //////////
    // extract address
    //////////

    // address string is in hex format (starts with '0x')
    if ((sAddr[0] == '0') && ((sAddr[1] == 'x') || (sAddr[1] == 'X'))) {

      // check for valid characters 0-9, A-F
      for (i=2; i<strlen(sAddr); i++) {
        if (!isxdigit(sAddr[i])) {
          g_hexFileErrors = STM8GAL_HEXFILE_INVALID_CHAR;
          console_print(STDERR, "Line %u in table file: hex address '%s' contains invalid character ('%c')", linecount, sAddr, sAddr[i]);
          return(g_hexFileErrors);
        }
      }

      // get address
      sscanf(sAddr, "%" SCNx64, &addr);

    } // address is in hex format

    // address string is in decimal format
    else {

      // check for valid characters 0-9
      for (i=0; i<strlen(sAddr); i++) {
        if (!isdigit(sAddr[i])) {
          g_hexFileErrors = STM8GAL_HEXFILE_INVALID_CHAR;
          console_print(STDERR, "Line %u in table file: dec address '%s' contains invalid character ('%c')", linecount, sAddr, sAddr[i]);
          return(g_hexFileErrors);
        }
      }

      // get address
      sscanf(sAddr, "%" SCNu64, &addr);


    } // extract address


    //////////
    // extract value
    //////////

    // value string is in hex format (starts with '0x')
    if ((sValue[0] == '0') && ((sValue[1] == 'x') || (sValue[1] == 'X'))) {

      // check for valid characters 0-9, A-F
      for (i=2; i<strlen(sValue); i++) {
        if (!isxdigit(sValue[i])) {
          g_hexFileErrors = STM8GAL_HEXFILE_INVALID_CHAR;
          console_print(STDERR, "Line %u in table file: hex value '%s' contains invalid character ('%c')", linecount, sValue, sValue[i]);
          return(g_hexFileErrors);
        }
      }

      // get address
      sscanf(sValue, "%x", &val);

    } // address is in hex format

    // address string is in decimal format
    else {

      // check for valid characters 0-9
      for (i=0; i<strlen(sValue); i++) {
        if (!isdigit(sValue[i])) {
          g_hexFileErrors = STM8GAL_HEXFILE_INVALID_CHAR;
          console_print(STDERR, "Line %u in table file: dec value '%s' contains invalid character ('%c')", linecount, sValue, sValue[i]);
          return(g_hexFileErrors);
        }
      }

      // get address
      sscanf(sValue, "%d", &val);

    } // extract value

    // check for buffer overflow
    if (addr > (uint64_t) (LENIMAGEBUF-1L)) {
      g_hexFileErrors = STM8GAL_HEXFILE_INVALID_CHAR;
      console_print(STDERR, "Line %u in table file: buffer size exceeded (%dMB vs %dMB)", linecount, (int) (addr/1024L/1024L), (int) (LENIMAGEBUF/1024L/1024L));
      return(g_hexFileErrors);
    }

    // for printout store min/max address in file
    if (addr < addrStart)  addrStart = addr;
    if (addr > addrStop)   addrStop  = addr;

    // store data byte in buffer and set high byte
    imageBuf[addr] = (uint16_t) val | 0xFF00;
    numData++;

  } // while !EOF

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0/1024.0, addrStart, addrStop);
    else if (numData>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0, addrStart, addrStop);
    else if (numData>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_convertTxt



/**
   \fn STM8gal_HexFileErrors_t hexfile_convertBin(char *fileBuf, uint64_t lenFileBuf, uint64_t addrStart, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  fileBuf      memory buffer to read from
   \param[in]  lenFileBuf   length of memory buffer
   \param[in]  addrStart    address offset for binary import
   \param[out] imageBuf     RAM image of file. HB!=0 indicates content
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

   convert memory buffer containing binary data to memory image. Binary data contains no absolute addresses,
   just data. Therefor a starting address must also be provided.
*/
STM8gal_HexFileErrors_t hexfile_convertBin(char *fileBuf, uint64_t lenFileBuf, uint64_t addrStart, uint16_t *imageBuf, uint8_t verbose) {

  uint64_t  addrStop, numData;
  uint64_t  i;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  convert binary ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  convert binary data ... ");

  // calculate number of bytes and last address
  numData  = lenFileBuf;
  addrStop = addrStart + numData;

  // check for buffer overflow
  if (addrStop > (uint64_t) (LENIMAGEBUF-1L)) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_BUFFER_SIZE_EXCEEDED;
    console_print(STDERR, "Binary file conversion: buffer size exceeded (%dMB vs %dMB)", (int) (addrStop/1024L/1024L), (int) (LENIMAGEBUF/1024L/1024L));
    return(g_hexFileErrors);
  }

  // copy data and mark as set (HB=0xFF)
  for (i=0; i<numData; i++) {
    imageBuf[addrStart+i] = ((uint16_t) fileBuf[i]) | 0xFF00;
  }

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0/1024.0, addrStart, addrStop);
    else if (numData>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0, addrStart, addrStop);
    else if (numData>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_convertBin



/**
   \fn STM8gal_HexFileErrors_t hexfile_getImageSize(uint16_t *imageBuf, uint64_t scanStart, uint64_t scanStop, uint64_t *addrStart, uint64_t *addrStop, uint64_t *numData)

   \param[in]  imageBuf     memory image containing data. HB!=0 indicates content
   \param[in]  scanStart    start address for scan
   \param[in]  scanStop     end address for scan
   \param[out] addrStart    first address containing data (HB!=0x00)
   \param[out] addrStop     last address containing data (HB!=0x00)
   \param[out] numData      number of data bytes in image (HB!=0x00)

   \return communication status (STM8gal_BootloaderErrors_t)

   Get fist and last address and number of bytes in memory image. Defined data is indicated by HB!=0x00
*/
STM8gal_HexFileErrors_t hexfile_getImageSize(uint16_t *imageBuf, uint64_t scanStart, uint64_t scanStop, uint64_t *addrStart, uint64_t *addrStop, uint64_t *numData) {

  uint64_t   addr;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // simple checks of scan window
  if (scanStart > scanStop) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_INVALID;
    console_print(STDERR, "scan start address 0x%" PRIx64 " higher than end address 0x%" PRIx64, scanStart, scanStop);
    return(g_hexFileErrors);
  }
  if (scanStart > LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "scan start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, scanStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (scanStop > LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "scan end address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, scanStop, LENIMAGEBUF);
    return(g_hexFileErrors);
  }

  // loop though image and check for defined data (HB!=0x00)
  *addrStart = 0xFFFFFFFFFFFFFFFF;
  *addrStop  = 0x0000000000000000;
  *numData   = 0;
  for (addr=scanStart; addr<=scanStop; addr++) {

    // entry contains data (HB!=0x00)
    if (imageBuf[addr] & 0xFF00) {
      if (addr < *addrStart) *addrStart = addr;
      if (addr > *addrStop)  *addrStop  = addr;
      (*numData)++;
    }

  } // loop over image

  // return status
  return(g_hexFileErrors);

} // hexfile_getImageSize



/**
   \fn STM8gal_HexFileErrors_t hexfile_fillImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t value, uint8_t verbose)

   \param      imageBuf     memory image containing data. HB!=0 indicates content
   \param[in]  addrStart    starting address of filling window
   \param[in]  addrStop     topmost address of filling window
   \param[in]  value        value to write
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

   Fill memory image in specified window with specified value and set status to "defined" (HB=0xFF)
*/
STM8gal_HexFileErrors_t hexfile_fillImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t value, uint8_t verbose) {

  uint64_t  addr, numFilled;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  fill image ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  fill memory image ... ");

  // simple checks of scan window
  if (addrStart > addrStop) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_INVALID;
    console_print(STDERR, "start address 0x%" PRIx64 " higher than end address 0x%" PRIx64, addrStart, addrStop);
    return(g_hexFileErrors);
  }
  if (addrStart > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (addrStop > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "end address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStop, LENIMAGEBUF);
    return(g_hexFileErrors);
  }

  // loop over memory image and clear all data outside specified clipping window
  numFilled = 0;
  for (addr = addrStart; addr <= addrStop; addr++) {
    numFilled++;                                      // count filled bytes for output below
    imageBuf[addr] = ((uint16_t) value) | 0xFF00;     // HB=0x00 indicates data undefined, LB contains data
  }

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numFilled>1024*1024)
      console_print(STDOUT, "done, filled %1.1fMB with 0x%02x within 0x%" PRIx64 " - 0x%" PRIx64 "\n", (float) numFilled/1024.0/1024.0, value, addrStart, addrStop);
    else if (numFilled>1024)
      console_print(STDOUT, "done, filled %1.1fkB with 0x%02x within 0x%" PRIx64 " - 0x%" PRIx64 "\n", (float) numFilled/1024.0, value, addrStart, addrStop);
    else if (numFilled>0)
      console_print(STDOUT, "done, filled %dB with 0x%02x within 0x%" PRIx64 " - 0x%" PRIx64 "\n", (int) numFilled, value, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data filled\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_fillImage



/**
   \fn STM8gal_HexFileErrors_t hexfile_clipImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose)

   \param      imageBuf     memory image containing data. HB!=0 indicates content
   \param[in]  addrStart    starting address of clipping window
   \param[in]  addrStop     topmost address of clipping window
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

   Clip memory image to specified window, i.e. reset all data outside specified window to "undefined" (HB=0x00)
*/
STM8gal_HexFileErrors_t hexfile_clipImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose) {

  uint64_t  addr, numCleared;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  clip image ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  clip memory image ... ");

  // simple checks of scan window
  if (addrStart > addrStop) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "start address 0x%" PRIx64 " higher than end address 0x%" PRIx64, addrStart, addrStop);
    return(g_hexFileErrors);
  }
  if (addrStart > (uint64_t) LENIMAGEBUF)  {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (addrStop > (uint64_t) LENIMAGEBUF)  {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "end address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStop, LENIMAGEBUF);
    return(g_hexFileErrors);
  }

  // loop over memory image and clear all data outside specified clipping window
  numCleared = 0;
  for (addr = 0; addr < (uint64_t)LENIMAGEBUF; addr++) {
    if ((addr < addrStart) || (addr > addrStop)) {
      if (imageBuf[addr] & 0xFF00)
         numCleared++;                 // count deleted bytes for output below
      imageBuf[addr] = 0x0000;         // HB=0x00 indicates data undefined, LB contains data
    }
  }

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numCleared>1024*1024)
      console_print(STDOUT, "done, clipped %1.1fMB outside 0x%" PRIx64 " - 0x%" PRIx64 "\n", (float) numCleared/1024.0/1024.0, addrStart, addrStop);
    else if (numCleared>1024)
      console_print(STDOUT, "done, clipped %1.1fkB outside 0x%" PRIx64 " - 0x%" PRIx64 "\n", (float) numCleared/1024.0, addrStart, addrStop);
    else if (numCleared>0)
      console_print(STDOUT, "done, clipped %" PRId64 "B outside 0x%" PRIx64 " - 0x%" PRIx64 "\n", numCleared, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data cleared\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_clipImage



/**
   \fn STM8gal_HexFileErrors_t hexfile_cutImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose)

   \param      imageBuf     memory image containing data. HB!=0 indicates content
   \param[in]  addrStart    starting address of section to clear
   \param[in]  addrStop     topmost address of section to clear
   \param[in]  verbose      verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return communication status (STM8gal_BootloaderErrors_t)

   Cut data range from memory image, i.e. reset all data inside specified window to "undefined" (HB=0x00)
*/
STM8gal_HexFileErrors_t hexfile_cutImage(uint16_t *imageBuf, uint64_t addrStart, uint64_t addrStop, uint8_t verbose) {

  uint64_t  addr, numCleared;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  clear image ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  clear memory image ... ");

  // simple checks of scan window
  if (addrStart > addrStop) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "start address 0x%" PRIx64 " higher than end address 0x%" PRIx64, addrStart, addrStop);
    return(g_hexFileErrors);
  }
  if (addrStart > (uint64_t) LENIMAGEBUF)  {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (addrStop > (uint64_t) LENIMAGEBUF)  {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "end address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, addrStop, LENIMAGEBUF);
    return(g_hexFileErrors);
  }

  // loop over memory image and clear all data inside specified window
  numCleared = 0;
  for (addr=0; addr<(uint64_t)LENIMAGEBUF; addr++) {
    if ((addr >= addrStart) && (addr <= addrStop)) {
      if (imageBuf[addr] & 0xFF00)
         numCleared++;                 // count deleted bytes for output below
      imageBuf[addr] = 0x0000;         // HB=0x00 indicates data undefined, LB contains data
    }
  }

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numCleared>1024*1024)
      console_print(STDOUT, "done, cut %1.1fMB within 0x%" PRIx64 " - 0x%" PRIx64 "\n", (float) numCleared/1024.0/1024.0, addrStart, addrStop);
    else if (numCleared>1024)
      console_print(STDOUT, "done, cut %1.1fkB within 0x%" PRIx64 " - 0x%" PRIx64 "\n", (float) numCleared/1024.0, addrStart, addrStop);
    else if (numCleared>0)
      console_print(STDOUT, "done, cut %" PRId64 "B within 0x%" PRIx64 " - 0x%" PRIx64 "\n", numCleared, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data cut\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_cutImage



/**
   \fn STM8gal_HexFileErrors_t hexfile_copyImage(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose)

   \param      imageBuf          memory image containing data. HB!=0 indicates content
   \param[in]  sourceStart       starting address to copy from
   \param[in]  sourceStart       last address to copy from
   \param[in]  destinationStart  starting address to copy to
   \param[in]  verbose           verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return operation status (STM8gal_BootloaderErrors_t)

   Copy data section within image to new address. Data at old address is maintained (if sections don't overlap).
*/
STM8gal_HexFileErrors_t hexfile_copyImage(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose) {

  uint64_t  numCopied, i;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  copy data ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  copy image data ... ");

  // simple checks of scan window
  if (sourceStart > sourceStop) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_INVALID;
    console_print(STDERR, "source start address 0x%" PRIx64 " higher than end address 0x%" PRIx64, sourceStart, sourceStop);
    return(g_hexFileErrors);
  }
  if (sourceStart > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "source start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, sourceStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (sourceStop > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "source end address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, sourceStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (destinationStart > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "destination start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, destinationStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (destinationStart+(sourceStop-sourceStart+1) > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "destination end address 0x%" PRIx64 " exceeds buffer size 0x" PRIx64, destinationStart+(sourceStop-sourceStart+1), LENIMAGEBUF);
    return(g_hexFileErrors);
  }

  // get number of data to copy (HB!=0x00)
  numCopied = 0;
  for (i=sourceStart; i<=sourceStop; i++) {
    if (imageBuf[i] & 0xFF00)
      numCopied++;
  }

  // copy data within image
  memcpy((void*) &(imageBuf[destinationStart]), (void*) &(imageBuf[sourceStart]), (sourceStop-sourceStart+1)*sizeof(*imageBuf));


  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numCopied>1024*1024)
      console_print(STDOUT, "done, copied %1.1fMB from 0x%" PRIx64 " - 0x%" PRIx64 " to 0x%" PRIx64 "\n", (float) numCopied/1024.0/1024.0, sourceStart, sourceStop, destinationStart);
    else if (numCopied>1024)
      console_print(STDOUT, "done, copied %1.1fkB from 0x%" PRIx64 " - 0x%" PRIx64 " to 0x%" PRIx64 "\n", (float) numCopied/1024.0, sourceStart, sourceStop, destinationStart);
    else if (numCopied>0)
      console_print(STDOUT, "done, copied %" PRId64 "B from 0x%" PRIx64 " - 0x%" PRIx64 " to 0x%" PRIx64 "\n", numCopied, sourceStart, sourceStop, destinationStart);
    else
      console_print(STDOUT, "done, no data copied\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_copyImage



/**
   \fn STM8gal_HexFileErrors_t hexfile_moveImage(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose)

   \param      imageBuf          memory image containing data. HB!=0 indicates content
   \param[in]  sourceStart       starting address to move from
   \param[in]  sourceStart       last address to move from
   \param[in]  destinationStart  starting address to move to
   \param[in]  verbose           verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return operation status (STM8gal_BootloaderErrors_t)

   Move data section within image to new address. Data at old address is cleared.
*/
STM8gal_HexFileErrors_t hexfile_moveImage(uint16_t *imageBuf, uint64_t sourceStart, uint64_t sourceStop, uint64_t destinationStart, uint8_t verbose) {

  uint64_t  numMoved, i;
  uint16_t  *tmpImageBuf;   // temporary buffer

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // print message
  if (verbose == INFORM)
    console_print(STDOUT, "  move data ... ");
  else if (verbose == CHATTY)
    console_print(STDOUT, "  move image data ... ");

  // simple checks of scan window
  if (sourceStart > sourceStop) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_INVALID;
    console_print(STDERR, "source start address 0x%" PRIx64 " higher than end address 0x%" PRIx64, sourceStart, sourceStop);
    return(g_hexFileErrors);
  }
  if (sourceStart > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "source start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, sourceStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (sourceStop > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "source end address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, sourceStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (destinationStart > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "destination start address 0x%" PRIx64 " exceeds buffer size 0x%" PRIx64, destinationStart, LENIMAGEBUF);
    return(g_hexFileErrors);
  }
  if (destinationStart+(sourceStop-sourceStart+1) > (uint64_t) LENIMAGEBUF) {
    g_hexFileErrors = STM8GAL_HEXFILE_FILE_ADDRESS_EXCEEDS_BUFFER;
    console_print(STDERR, "destination end address 0x%" PRIx64 " exceeds buffer size 0x" PRIx64, destinationStart+(sourceStop-sourceStart+1), LENIMAGEBUF);
    return(g_hexFileErrors);
  }


  // get number of data to move (HB!=0x00)
  numMoved = 0;
  for (i=sourceStart; i<=sourceStop; i++) {
    if (imageBuf[i] & 0xFF00)
      numMoved++;
  }

  // allocate temporary buffer (required for overlapping windows
  if (!(tmpImageBuf = malloc(LENIMAGEBUF * sizeof(*tmpImageBuf))))
    Error("Cannot allocate image buffer, try reducing LENIMAGEBUF");

  // copy data from image to temporary buffer
  memcpy((void*) &(tmpImageBuf[sourceStart]), (void*) &(imageBuf[sourceStart]), (sourceStop-sourceStart+1)*sizeof(*imageBuf));

  // remove old data from image
  if (hexfile_cutImage(imageBuf, sourceStart, sourceStop, MUTE) != STM8GAL_HEXFILE_NO_ERROR)
    return(g_hexFileErrors);

  // copy data from temporary buffer to image
  memcpy((void*) &(imageBuf[destinationStart]), (void*) &(tmpImageBuf[sourceStart]), (sourceStop-sourceStart+1)*sizeof(*imageBuf));

  // release temporary buffer again
  free(tmpImageBuf);

  // print message
  if (verbose == INFORM) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numMoved>1024*1024)
      console_print(STDOUT, "done, moved %1.1fkB from 0x%" PRIx64 " - 0x%" PRIx64 " to 0x%" PRIx64 "\n", (float) numMoved/1024.0/1024.0, sourceStart, sourceStop, destinationStart);
    else if (numMoved>1024)
      console_print(STDOUT, "done, moved %1.1fkB from 0x%" PRIx64 " - 0x%" PRIx64 " to 0x%" PRIx64 "\n", (float) numMoved/1024.0, sourceStart, sourceStop, destinationStart);
    else if (numMoved>0)
      console_print(STDOUT, "done, moved %" PRId64 "B from 0x%" PRIx64 " - 0x%" PRIx64 " to 0x%" PRIx64 "\n", numMoved, sourceStart, sourceStop, destinationStart);
    else
      console_print(STDOUT, "done, no data moved\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_moveImage



/**
   \fn STM8gal_HexFileErrors_t hexfile_exportS19(char *filename, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  filename    name of output file
   \param[in]  imageBuf    memory image. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return operation status (STM8gal_BootloaderErrors_t)

   export RAM image to file in s19 hexfile format. For description of
   Motorola S19 file format see http://en.wikipedia.org/wiki/SREC_(file_format)
*/
STM8gal_HexFileErrors_t hexfile_exportS19(char *filename, uint16_t *imageBuf, uint8_t verbose) {

  FILE      *fp;               // file pointer
  const int maxLine = 32;      // max. length of data line
  uint8_t   data;              // value to store
  uint32_t  chk;               // checksum
  uint64_t  addr, addrStart, addrStop, numData;  // image data range
  char      *shortname;        // filename w/o path
  int       j;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // strip path from filename for readability
  #if defined(WIN32) || defined(WIN64)
    shortname = strrchr(filename, '\\');
  #else
    shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    console_print(STDOUT, "  export '%s' ... ", shortname);
  else if (verbose == INFORM)
    console_print(STDOUT, "  export S19 file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    console_print(STDOUT, "  export Motorola S19 file '%s' ... ", shortname);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp) {
    g_hexFileErrors = STM8GAL_HEXFILE_FAILED_CREATE_FILE;
    console_print(STDERR, "Failed to create file %s", filename);
    return(g_hexFileErrors);
  }

  // start with dummy header line to avoid 'srecord' warning
  fprintf(fp, "S00F000068656C6C6F202020202000003C\n");

  // get min/max addresses and number of bytes (HB!=0x00) in image
  if ( hexfile_getImageSize(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR)
    return(g_hexFileErrors);

  // store in lines of 32B
  addr = addrStart;
  while (addr <= addrStop) {

    // find next data byte (=start address of next block)
    while (((imageBuf[addr] & 0xFF00) == 0) && (addr <= addrStop))
      addr++;
    uint64_t addrBlock = addr;

    // end address reached -> done
    if (addr > addrStop)
      break;

    // set length of next data block: max 128B and align with 128 for speed (see UM0560 section 3.4)
    int lenBlock = 1;
    while ((lenBlock < maxLine) && ((addr+lenBlock) <= addrStop) && (imageBuf[addr+lenBlock] & 0xFF00) && ((addr+lenBlock) % maxLine)) {
      lenBlock++;
    }
    //console_print(STDOUT, "0x%04x   0x%04x   %d\n", addrBlock, addrBlock+lenBlock-1, lenBlock);


    ///////
    // save data in next line, see http://en.wikipedia.org/wiki/SREC_(file_format)
    ///////

    // save data, accound for address width
    if (addrStop <= 0xFFFF) {
      fprintf(fp, "S1%02X%04X", lenBlock+3, (int) addrBlock);        // 16-bit address: 2B addr + data + 1B chk
      chk = (uint8_t) (lenBlock+3) + (uint8_t) addrBlock + (uint8_t) (addrBlock >> 8);
    }
    else if (addrStop <= 0xFFFFFF) {
      fprintf(fp, "S2%02X%06X", lenBlock+4, (int) addrBlock);        // 24-bit address: 3B addr + data + 1B chk
      chk = (uint8_t) (lenBlock+4) + (uint8_t) addrBlock + (uint8_t) (addrBlock >> 8) + (uint8_t) (addrBlock >> 16);
    }
    else {
      fprintf(fp, "S3%02X%08X", lenBlock+5, (int) addrBlock);        // 32-bit address: 4B addr + data + 1B chk
      chk = (uint8_t) (lenBlock+5) + (uint8_t) addrBlock + (uint8_t) (addrBlock >> 8) + (uint8_t) (addrBlock >> 16) + (uint8_t) (addrBlock >> 24);
    }
    for (j=0; j<lenBlock; j++) {
      data = (uint8_t) (imageBuf[addrBlock+j] & 0x00FF);
      chk += data;
      fprintf(fp, "%02X", data);
    }
    chk = ((chk & 0xFF) ^ 0xFF);
    fprintf(fp, "%02X\n", chk);

    // go to next potential block
    addr += lenBlock;

  } // loop over address range

  // attach appropriate termination record, according to type of data records used
  if (addrStop <= 0xFFFF)
    fprintf(fp, "S9030000FC\n");        // 16-bit addresses
  else if (addrStop <= 0xFFFFFF)
    fprintf(fp, "S804000000FB\n");      // 24-bit addresses
  else
    fprintf(fp, "S70500000000FA\n");    // 32-bit addresses

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0/1024.0, addrStart, addrStop);
    if (numData>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0, addrStart, addrStop);
    else if (numData>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_exportS19



/**
   \fn STM8gal_HexFileErrors_t hexfile_exportIHex(char *filename, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  filename    name of output file
   \param[in]  imageBuf    memory image. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return operation status (STM8gal_BootloaderErrors_t)

   export RAM image to file in Intel hexfile format. For description of
   Intel hex file format see http://en.wikipedia.org/wiki/Intel_HEX
*/

STM8gal_HexFileErrors_t hexfile_exportIHex(char *filename, uint16_t *imageBuf, uint8_t verbose) {

  FILE      *fp;               // file pointer
  const int maxLine = 32;      // max. length of data line
  uint8_t   data;              // value to store
  uint8_t   chk;               // checksum
  uint64_t  addrStart, addrStop, numData;  // image data range
  char      *shortname;        // filename w/o path
  uint8_t   useEla = 0;        // whether ELA records needed
  int64_t   addrEla;           // ELA record address
  uint8_t   j;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // strip path from filename for readability
  #if defined(WIN32) || defined(WIN64)
    shortname = strrchr(filename, '\\');
  #else
    shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    console_print(STDOUT, "  export '%s' ... ", shortname);
  else if (verbose == INFORM)
    console_print(STDOUT, "  export IHX file '%s' ... ", shortname);
  else if (verbose == CHATTY)
    console_print(STDOUT, "  export Intel HEX file '%s' ... ", shortname);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp) {
    g_hexFileErrors = STM8GAL_HEXFILE_FAILED_CREATE_FILE;
    console_print(STDERR, "Failed to create file %s", filename);
    return(g_hexFileErrors);
  }

  // get min/max addresses and number of bytes (HB!=0x00) in image
  if ( hexfile_getImageSize(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR)
    return(g_hexFileErrors);

  // use ELA records if address range is greater than 16 bits
  if(addrStop > 0xFFFF) {
    useEla = 1;
    addrEla = -1;
  }

  uint64_t addr = addrStart;
  while(addr <= addrStop) {
    // find next data byte (=start address of next block)
    while(((imageBuf[addr] & 0xFF00) == 0) && (addr <= addrStop)) addr++;
    uint64_t addrBlock = addr;

    // end address reached -> done
    if(addr > addrStop) break;

    // set length of next data block: max 128B and align with 128 for speed (see UM0560 section 3.4)
    uint8_t lenBlock = 1;
    while((lenBlock < maxLine) && ((addr+lenBlock) <= addrStop) && (imageBuf[addr+lenBlock] & 0xFF00) && ((addr+lenBlock) % maxLine)) {
      lenBlock++;
    }

    // write ELA record if upper 16-bits of block addr is different than last ELA addr
    if(useEla && addrEla != (addrBlock >> 16)) {
      addrEla = addrBlock >> 16;
      chk = ~(0x02 + 0x04 + (uint8_t)addrEla + (uint8_t)(addrEla >> 8)) + 1;
      fprintf(fp, ":02000004%04X%02X\n", (uint16_t)addrEla, chk);
    }

    // write the data record
    fprintf(fp, ":%02X%04X00", lenBlock, (uint16_t)addrBlock);
    chk = lenBlock + (uint8_t)addrBlock + (uint8_t)(addrBlock >> 8);
    for (j = 0; j < lenBlock; j++) {
      data = (uint8_t)(imageBuf[addrBlock+j] & 0x00FF);
      chk += data;
      fprintf(fp, "%02X", data);
    }
    chk = ~chk + 1;
    fprintf(fp, "%02X\n", chk);

    // go to next potential block
    addr += lenBlock;
  } // loop over address range

  // output end-of-file record
  fprintf(fp, ":00000001FF\n");

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0/1024.0, addrStart, addrStop);
    if (numData>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0, addrStart, addrStop);
    else if (numData>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_exportIHex



/**
   \fn STM8gal_HexFileErrors_t hexfile_exportTxt(char *filename, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  filename    name of output file or stdout ('console')
   \param[in]  imageBuf    memory image. HB!=0 indicates content. Index 0 corresponds to addrStart
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return operation status (STM8gal_BootloaderErrors_t)

   export RAM image to file with plain text table (hex addr / hex data)
*/
STM8gal_HexFileErrors_t hexfile_exportTxt(char *filename, uint16_t *imageBuf, uint8_t verbose) {

  FILE      *fp;               // file pointer
  uint64_t  addrStart, addrStop, numData;  // image data range
  char      *shortname;        // filename w/o path
  bool      flagFile = true;   // output to file or console?
  uint64_t  i;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // output to stdout
  if (!strcmp(filename, "console")) {
    flagFile = false;
    fp = stdout;
    if (verbose > MUTE)
      console_print(STDOUT, "  print memory\n");
  }

  // output to file
  else {
    flagFile = true;

    // strip path from filename for readability
    #if defined(WIN32) || defined(WIN64)
      shortname = strrchr(filename, '\\');
    #else
      shortname = strrchr(filename, '/');
    #endif
    if (!shortname)
      shortname = filename;
    else
      shortname++;

    // print message
    if (verbose == SILENT)
      console_print(STDOUT, "  export '%s' ... ", shortname);
    else if (verbose == INFORM)
      console_print(STDOUT, "  export table '%s' ... ", shortname);
    else if (verbose == CHATTY)
      console_print(STDOUT, "  export ASCII table to file '%s' ... ", shortname);

    // open output file
    fp=fopen(filename,"wb");
    if (!fp) {
      g_hexFileErrors = STM8GAL_HEXFILE_FAILED_CREATE_FILE;
      console_print(STDERR, "Failed to create file %s", filename);
      return(g_hexFileErrors);
    }

  } // output to file

  // output header
  if (flagFile)
    fprintf(fp, "# address	value\n");
  else
    fprintf(fp, "    address	value\n");

  // get min/max addresses and number of bytes (HB!=0x00) in image
  if ( hexfile_getImageSize(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR)
    return(g_hexFileErrors);

  // output each defined value (HB!=0x00) in a separate line (addr \t value)
  for (i=addrStart; i<=addrStop; i++) {
    if (imageBuf[i] & 0xFF00) {
      if (!flagFile)
        fprintf(fp,"    ");
      fprintf(fp, "0x%" PRIx64 "	0x%02x\n", i, (imageBuf[i] & 0xFF));
      //console_print(STDOUT, "0x%" PRIx64 "   0x%04x   0x%02x\n", i, imageBuf[i], (imageBuf[i] & 0xFF));
    }
  }

  // close output file
  fflush(fp);
  if (flagFile)
    fclose(fp);
  else
    fprintf(fp,"  ");

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (numData>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0/1024.0, addrStart, addrStop);
    if (numData>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) numData/1024.0, addrStart, addrStop);
    else if (numData>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) numData, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_exportTxt



/**
   \fn STM8gal_HexFileErrors_t hexfile_exportBin(char *filename, uint16_t *imageBuf, uint8_t verbose)

   \param[in]  filename    name of output file
   \param[in]  imageBuf    memory image. HB!=0 indicates content
   \param[in]  verbose     verbosity level (0=MUTE, 1=SILENT, 2=INFORM, 3=CHATTY)

   \return operation status (STM8gal_BootloaderErrors_t)

   export RAM image to binary file. Note that start address is not stored, and that
   binary format does not allow for "holes" in the file, i.e. undefined data is stored as 0x00.
*/
STM8gal_HexFileErrors_t hexfile_exportBin(char *filename, uint16_t *imageBuf, uint8_t verbose) {

  FILE      *fp;               // file pointer
  uint64_t  addr, addrStart, addrStop, numData;  // address range to consider
  uint64_t  countByte;         // number of actually exported bytes
  uint8_t   val;

  // set default
  g_hexFileErrors = STM8GAL_HEXFILE_NO_ERROR;

  // strip path from filename for readability
  #if defined(WIN32) || defined(WIN64)
    const char *shortname = strrchr(filename, '\\');
  #else
    const char *shortname = strrchr(filename, '/');
  #endif
  if (!shortname)
    shortname = filename;
  else
    shortname++;

  // print message
  if (verbose == SILENT)
    console_print(STDOUT, "  export '%s' ... ", shortname);
  else if (verbose == INFORM)
    console_print(STDOUT, "  export binary '%s' ... ", shortname);
  else if (verbose == CHATTY)
    console_print(STDOUT, "  export binary to file '%s' ... ", shortname);

  // open output file
  fp=fopen(filename,"wb");
  if (!fp) {
    g_hexFileErrors = STM8GAL_HEXFILE_FAILED_CREATE_FILE;
    console_print(STDERR, "Failed to create file %s", filename);
    return(g_hexFileErrors);
  }

  // get address range containing data (HB!=0x00)
  if ( hexfile_getImageSize(imageBuf, 0, LENIMAGEBUF, &addrStart, &addrStop, &numData) != STM8GAL_HEXFILE_NO_ERROR)
    return(g_hexFileErrors);

  // store every value in address range. Undefined values are set to 0x00
  countByte = 0;
  for (addr=addrStart; addr<=addrStop; addr++) {
    if (imageBuf[addr] & 0xFF00)
      val = (uint8_t) (imageBuf[addr] & 0x00FF);
    else
      val = 0x00;
    fwrite(&val,sizeof(val), 1, fp); // write byte per byte (image is 16-bit)
    //console_print(STDOUT, "0x%04x   0x%04x   0x%02x\n", addr, imageBuf[addr], (imageBuf[addr] & 0xFF));
    countByte++;
  }

  // close output file
  fflush(fp);
  fclose(fp);

  // print message
  if ((verbose == SILENT) || (verbose == INFORM)) {
    console_print(STDOUT, "done\n");
  }
  else if (verbose == CHATTY) {
    if (countByte>1024*1024)
      console_print(STDOUT, "done (%1.1fMB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) countByte/1024.0/1024.0, addrStart, addrStop);
    else if (countByte>1024)
      console_print(STDOUT, "done (%1.1fkB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (float) countByte/1024.0, addrStart, addrStop);
    else if (countByte>0)
      console_print(STDOUT, "done (%dB in 0x%" PRIx64 " - 0x%" PRIx64 ")\n", (int) countByte, addrStart, addrStop);
    else
      console_print(STDOUT, "done, no data\n");
  }

  // return status
  return(g_hexFileErrors);

} // hexfile_exportBin

/**
   \fn STM8gal_HexFileErrors_t hexfile_GetLastError(void)

   \return last error in the Hexfile module


   The return of the last operation status
*/
STM8gal_HexFileErrors_t hexfile_GetLastError(void)
{
  return(g_hexFileErrors);
}

/**
  \fn const char * hexfile_GetLastErrorString(void)
   
  \return last error string in the hexfile module
*/
const char * hexfile_GetLastErrorString(void)
{
    return(g_hexFileErrorStrings[hexfile_GetLastError()]);
}

// end of file
